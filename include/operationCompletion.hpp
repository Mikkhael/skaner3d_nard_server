#pragma once

#include "asiolib/utils.hpp"

enum class CustomError {
    None,
    FileOpen,
    FileRead,
    FileWrite,
    Critical,
    Illformated
};

class OperationCompletion{
public:
    struct Result{
        uint32_t code;
        std::string category;
        std::string message;
        
        Result() = default;
        Result(uint32_t code, std::string category, std::string message)
            : code(code), category(category), message(message)
        {}
        Result(const Error& error){
            if(!error){
                code = 0;
                return;
            }
            code = error.value();
            category = error.category().name();
            message = error.message();
        }
        Result(const CustomError error){
            category = "custom";
            switch(error){
                case CustomError::None :        code = 0; message = ""; break;
                case CustomError::FileOpen :    code = 1; message = "Cannot open file"; break;
                case CustomError::FileRead :    code = 2; message = "Cannot read from file"; break;
                case CustomError::FileWrite :   code = 3; message = "Cannot write to file"; break;
                case CustomError::Critical :    code = 4; message = "Critical server error"; break;
                case CustomError::Illformated : code = 5; message = "Received data is illformated"; break;
            }
        }
        operator bool() const { return code != 0; }
    };
    
private:
    std::mutex mutex;
    bool isBusyFlag = false;
    Result result;
    std::function<void(Result)> completionHandler = [](const Result&){};
    
public:
    bool setBusy(){
        std::lock_guard<std::mutex> lock{mutex};
        if(isBusyFlag){
            return true;
        }
        isBusyFlag = true;
        result = CustomError::None;
        return false;
    }
    bool isBusy(){
        std::lock_guard<std::mutex> lock{mutex};
        return isBusyFlag;
    }
    void setResult(const Result& result){
        std::lock_guard<std::mutex> lock{mutex};
        this->result = result;
    }
    Result getResult(){
        std::lock_guard<std::mutex> lock{mutex};
        return result;
    }
    template<typename T>
    void setCompletionHandler(T&& handler){
        std::lock_guard<std::mutex> lock{mutex};
        completionHandler = handler;
    }
    void complete(){
        std::lock_guard<std::mutex> lock{mutex};
        if(isBusyFlag){
            isBusyFlag = false;
            completionHandler(result);
        }
    }
    
    ~OperationCompletion(){
        if(isBusyFlag){
            isBusyFlag = false;
            completionHandler(result);
        }
    }
};

inline std::ostream& operator<<(std::ostream& os, const OperationCompletion::Result& result){
    return os << result.category << ':' << result.code << " - " << result.message;
}
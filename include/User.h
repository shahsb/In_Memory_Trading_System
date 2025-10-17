#pragma once

#include "TradingSystemCore.h"

namespace TradingSystem {

    class User {
    private:
        UserId userId_;
        std::string userName_;
        std::string phoneNumber_;
        std::string emailId_;
        
    public:
        User(const UserId& userId, const std::string& userName, 
             const std::string& phoneNumber, const std::string& emailId);
        
        const UserId& getUserId() const;
        const std::string& getUserName() const;
        const std::string& getPhoneNumber() const;
        const std::string& getEmailId() const;
        
        bool isValid() const;
    };

} // namespace TradingSystem

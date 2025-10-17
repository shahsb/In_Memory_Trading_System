#include "../include/User.h"

namespace TradingSystem {

    User::User(const UserId& userId, const std::string& userName, 
             const std::string& phoneNumber, const std::string& emailId)
        : userId_(userId), userName_(userName), 
          phoneNumber_(phoneNumber), emailId_(emailId) {}
    
    const UserId& User::getUserId() const { return userId_; }
    const std::string& User::getUserName() const { return userName_; }
    const std::string& User::getPhoneNumber() const { return phoneNumber_; }
    const std::string& User::getEmailId() const { return emailId_; }
    
    bool User::isValid() const {
        return !userId_.empty() && !userName_.empty() && 
               !phoneNumber_.empty() && !emailId_.empty();
    }

} // namespace TradingSystem

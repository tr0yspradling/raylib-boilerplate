#pragma once

#include <cstdint>
#include <string>

namespace shared::net {

enum class AuthMode : uint8_t {
    DevInsecure,
    BackendToken,
};

struct AuthRequest {
    std::string playerName;
    std::string token;
    std::string sessionResumeToken;
};

struct AuthContext {
    std::string remoteAddress;
    uint64_t transportUserData = 0;
};

struct AuthDecision {
    bool accepted = false;
    uint64_t stableAccountId = 0;
    std::string rejectionReason;
    bool sessionResumeGranted = false;
};

class IAuthProvider {
public:
    virtual ~IAuthProvider() = default;
    virtual AuthDecision Validate(const AuthRequest& request, const AuthContext& context) = 0;
};

class DevAuthProvider final : public IAuthProvider {
public:
    AuthDecision Validate(const AuthRequest& request, const AuthContext&) override {
        AuthDecision decision;
        if (request.playerName.empty()) {
            decision.accepted = false;
            decision.rejectionReason = "player name required";
            return decision;
        }

        decision.accepted = true;
        decision.stableAccountId = std::hash<std::string>{}(request.playerName);
        decision.sessionResumeGranted = false;
        return decision;
    }
};

// Production auth integration is backend-dependent. This scaffold keeps a
// clean interface for token verification without claiming it is implemented.
class BackendTokenAuthProvider final : public IAuthProvider {
public:
    AuthDecision Validate(const AuthRequest&, const AuthContext&) override {
        return {
            .accepted = false,
            .stableAccountId = 0,
            .rejectionReason = "backend token auth not configured",
            .sessionResumeGranted = false,
        };
    }
};

}  // namespace shared::net

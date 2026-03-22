#include "SkillManager.h"

void SkillManager::createSkills(std::vector<Player>& players) {
    for (auto& player : players) {
        switch (player.getSkillName()) {
            case SkillName::DELIVERANCE:
                skills.push_back(std::make_unique<SkillDeliverance>(player.getId()));
                std::cout << "[createSkills] Created [Deliverance] for player " << player.getId() << std::endl;
                break;
            case SkillName::NEURALGAMBIT:
                skills.push_back(std::make_unique<SkillNeuralGambit>(player.getId()));
                std::cout << "[createSkills] Created [NeuralGambit] for player " << player.getId() << std::endl;
                break;
            case SkillName::FATALDEAL:
                skills.push_back(std::make_unique<SkillFatalDeal>(player.getId()));
                std::cout << "[createSkills] Created [FatalDeal] for player " << player.getId() << std::endl;
                break;
            default:
                std::cout << "[createSkills] Skill: " << player.skillNameToString() << " does not exist" << std::endl;
        }
    }
}

SkillExecutionResult SkillManager::processSkill(SkillContext& context) {
    SkillExecutionResult result;
    for (auto& skill : skills) {
        //find skill that the user is owning
        if (skill->getUserId() == context.user.getId()) {
            std::cout << "[processSkill] Using " << skill->skillNameToString() << std::endl;
            //check its conditions
            result.errorMsg = skill->canUse(context);
            if (result.errorMsg=="") {
                //execute skill -> pure logic, just modifies game
                bool success = skill->execute(context);
                //return result 
                if(success){
                    result.name = skill->getSkillName();
                    return result;
                }
                else {
                    result.errorMsg = "Skill execution failed";
                    result.name = SkillName::UNDEFINED;
                    return result;
                }
            }
            std::cout << "[processSkill] " << result.errorMsg << std::endl;
            result.name = SkillName::UNDEFINED;
            return result;
        }
    }
    result.errorMsg = "SKILL DOES NOT EXIST";
    std::cout << "[processSkill] Skill does not exist" << std::endl;
    result.name = SkillName::UNDEFINED;
    return result;
}

std::string SkillManager::preValidateSkill(int playerId, const GameState& state) {
    for (auto& skill : skills) {
        if (skill->getUserId() == playerId) {
            return skill->canActivate(state);
        }
    }
    return "SKILL DOES NOT EXIST";
}

int SkillManager::getSkillUses(SkillName name) {
    for (auto& skill : skills) {
        if (skill->getSkillName() == name) {
            return skill->getUses();
        }
    }
    return 100;
}

void SkillManager::resetSkillUses(std::vector<Player>& players) {
    std::cout << "[SkillManager] Reset all skills' uses" << std::endl;
    for (auto& skill : skills) {
        std::cout << "player:" << skill->getUserId() << std::endl;
        skill->resetUses();
    }
}

std::vector<std::pair<int, SkillName>> SkillManager::skillPassiveHandler(
    ReactiveTrigger trigger, const ReactiveContext& ctx,
    GameState& state, int priorityPlayerId, int numPlayers) {
    std::vector<std::pair<int, SkillName>> qualified;
    std::cout << "[SkillManager] Activating skill passive..." << std::endl;
    for (auto& skill : skills) {
        if (skill->activatePassive(state)) {
            qualified.push_back({skill->getUserId(), skill->getSkillName()});
            std::cout << "[SkillManager] Skill passive activated: "
                      << skill->skillNameToString() << " (P"
                      << skill->getUserId() << ")" << std::endl;
        }
    }
        // Sort: priorityPlayerId first, then by player ID wrapping around
    std::sort(
        qualified.begin(), qualified.end(),
        [priorityPlayerId, numPlayers](const auto& a, const auto& b) {
            if (a.first == priorityPlayerId && b.first != priorityPlayerId) return true;
            if (b.first == priorityPlayerId && a.first != priorityPlayerId) return false;
            // Wrap-around order from priorityPlayerId
            int da = (a.first - priorityPlayerId + numPlayers) % numPlayers;
            int db = (b.first - priorityPlayerId + numPlayers) % numPlayers;
            return da < db;
        }
    );
    return qualified;
}

std::vector<std::pair<int, SkillName>> SkillManager::getReactiveSkills(
        ReactiveTrigger trigger, const ReactiveContext& ctx,
        const GameState& state, int priorityPlayerId, int numPlayers) {

    std::vector<std::pair<int, SkillName>> qualified;

    for (auto& skill : skills) {
        if (skill->getReactiveTrigger() == trigger && skill->canReact(ctx, state)) {
            qualified.push_back({skill->getUserId(), skill->getSkillName()});
            std::cout << "[SkillManager] Reactive skill qualified: "
                      << skill->skillNameToString() << " (P"
                      << skill->getUserId() << ")" << std::endl;
        }
    }

    // Sort: priorityPlayerId first, then by player ID wrapping around
    std::sort(
        qualified.begin(), qualified.end(),
        [priorityPlayerId, numPlayers](const auto& a, const auto& b) {
            if (a.first == priorityPlayerId && b.first != priorityPlayerId) return true;
            if (b.first == priorityPlayerId && a.first != priorityPlayerId) return false;
            // Wrap-around order from priorityPlayerId
            int da = (a.first - priorityPlayerId + numPlayers) % numPlayers;
            int db = (b.first - priorityPlayerId + numPlayers) % numPlayers;
            return da < db;
        }
    );

    return qualified;
}

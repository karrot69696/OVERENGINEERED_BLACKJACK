#ifndef SKILLDECK_H
#define SKILLDECK_H
#include <vector>
#include <algorithm>
#include <random>
#include <iostream>
enum class SkillName {
    DELIVERANCE,
    NEURALGAMBIT,
    MULTIVERSE,
    CLONE,
    BOOGIEWOOGIE,
    LOOKMAXXING,
    SIGMA
};
class SkillDeck {
    private:
        std::vector<SkillName> skills;
    public:
        SkillDeck(){
            for (int i = 0; i <= 6; i++) {
                skills.emplace_back(
                    static_cast<SkillName>(i)
                );
            }
        };
        void shuffle(){
            std::random_device rd;
            std::mt19937 g(rd());
            std::shuffle(skills.begin(), skills.end(), g);
        };
        SkillName drawSkill(){
            if (skills.empty()) {
                throw std::out_of_range("No skills left in the skill deck");
            }
            SkillName drawnSkill = skills.front();
            skills.erase(skills.begin());
            return drawnSkill;
        };
        void clearCards(){
            skills.clear();
        }

};

#endif
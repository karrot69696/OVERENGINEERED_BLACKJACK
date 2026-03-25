#ifndef SKILLDECK_H
#define SKILLDECK_H
#include <vector>
#include <algorithm>
#include <random>
#include <iostream>
enum class SkillName {
    DELIVERANCE,
    NEURALGAMBIT,
    FATALDEAL,
    CHRONOSPHERE,
    DESTINYDEFLECT,
    BOOGIEWOOGIE,
    LOOKMAXXING,
    SIGMA,
    UNDEFINED
};
class SkillDeck {
    private:
        std::vector<SkillName> skills;
    public:
        SkillDeck(){};
        void buildDeck(int numPlayers){
            if (numPlayers>=7){
                throw std::out_of_range("Too many players");
            }
            for (int i = 0; i < numPlayers; i++){
                skills.push_back(SkillName(i));
            }
        }
        void buildDeck(){
            skills.push_back(SkillName::NEURALGAMBIT);
            skills.push_back(SkillName::FATALDEAL);
            skills.push_back(SkillName::CHRONOSPHERE);
            skills.push_back(SkillName::DELIVERANCE);
            skills.push_back(SkillName::DESTINYDEFLECT);
        }
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
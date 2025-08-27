#ifndef ENUMS_HPP
#define ENUMS_HPP

class Enums
{
public:
    Enums() {}
    ~Enums() {}


    enum LanguageOptions
    {
        English = 0,
        French,
        Japanese,
        Korean
    };

    enum CookingSelection
    {
        Rice = 0,
        Pancake,
        Dumpling,
        Fish
    };

    enum ScreenID
    {
        Settings = 0,
        Cooking
    };
};

#endif

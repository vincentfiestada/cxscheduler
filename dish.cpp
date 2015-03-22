#pragma once

#include <string>

using namespace std;

class Dish
{
    public:
    Dish(string name, int arrival)
    {
        Name = name; // MUST match filename for recipe
        ArrivalTime = arrival;
    }
    // Getters and Setters
    string GetName()
    {
        return Name;
    }
    void SetName(string n)
    {
        Name = n;
    }
    int GetArrival()
    {
        return ArrivalTime;
    }

    private:
    string Name;
    int ArrivalTime;
};

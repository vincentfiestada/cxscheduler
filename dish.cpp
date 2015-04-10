#pragma once

#include <string>
#define COOK 0
#define PREP 1
#define MAX_PRIORITY 10

using namespace std;

enum dish_state {READY, PREPPING, ONSTOVE, NOTARRIVED, MOVING, DONE};

class Task
{
public:
    Task(int type, int duration)
    {
        _type = type; // COOK or PREP
        _time = duration; // "seconds" remaining before task completion
    }
    int GetType() // getter of type property
    {
        return _type;
    }
    string GetStringType() // getter of type as string
    {
        return (_type == COOK) ? "Cook" : "Prep";
    }
    int GetTime() // getter for time property
    {
        return _time;
    }
    void Step() // execute 1 "second" of the task
    {
        _time--;
    }
    bool IsDone() // return true if the task is done
    {
        return (_time <= 0);
    }
private:
    int _type;
    int _time;
};

class Dish
{
public:
    /*
     * Constructor
     */
    Dish(string name, int arrival, int priority)
    {
        _name = name; // MUST match filename for recipe
        _arrivalTime = arrival; // "second" at which this dish is due to arrive
        _waitingTime = 0; // hasn't waited yet
        _priority = priority; // priority of dish used for scheduling algorithm
        _state = NOTARRIVED; // initial state is NotArrived always
    }

    // Getters and Setters
    string GetName() // getter for name
    {
        return _name;
    }
    void SetName(string n) // setter for name
    {
        _name = n;
    }
    int GetArrival() // getter for arrival time
    {
        return _arrivalTime;
    }
    int GetWaitingTime() // getter for waiting time
    {
        return _waitingTime;
    }
    void Wait() // incrementer for waiting time
    {
        _waitingTime++;
    }
    int GetPriority() // getter for priority
    {
        return _priority;
    }
    void SetPriority(int p) // setter for priority
    {
        // if p is less than zero, p = 0
        // elseif p is greater than MAX_PRIORITY (10), p = MAX_PRIORITY
        // else p is p
        p = (p < 0) ? 0 : (p > MAX_PRIORITY) ? MAX_PRIORITY : p;
        _priority = p;
    }
    dish_state GetState() // getter for state
    {
        return _state;
    }
    void SetState(dish_state s) // setter for state
    {
        _state = s;
    }
    vector<Task>& GetRecipe() // getter for recipe (returns reference)
    {
        return _recipe;
    }
    Task * GetNextTask() // get next unfinished task in recipe (reference)
    {
        for (int i = 0; i < _recipe.size(); i++)
        {
            // return the first task in recipe that is not yet done
            if (_recipe[i].IsDone() == false)
            {
                return &(_recipe.at(i));
            }
        }
        return NULL;
    }
    bool IsDone() // returns true if all tasks are done
    {
        // if all tasks are done, dish is done too
        return (this->GetNextTask() == NULL) ? true : false;
    }
    friend ostream& operator << (ostream &out, Dish &d)
    {
        out << d.GetName() << "(";
        Task * t = d.GetNextTask();
        if (t)
        {
            out << t->GetStringType() << " - " << t->GetTime();
        }
        else
        {
            out << "Done";
        }
        out << ")";
        return out;
    }

private:
    string _name;
    int _arrivalTime;
    int _waitingTime; // amount of time spent in Ready queue
    int _priority;
    dish_state _state;
    vector<Task> _recipe;
};

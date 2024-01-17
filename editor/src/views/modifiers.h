#ifndef modifiers_h__
#define modifiers_h__

#include "modifiers/modifier.h"

#include <vector>
#include <string>


struct ModifiersView
{
    std::vector<Modifier *> activeModifiers;
    std::vector<Modifier *> modifiers;

    ModifiersView();
    void gui();
    void toolbar();
};

#endif // modifiers_h__

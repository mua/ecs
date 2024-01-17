#include "modifier.h"

struct SelectionModifier: Modifier
{
    bool process(Handle viewport) override;
    bool isAvailable() override;
};

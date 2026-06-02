#pragma once

#include "../parser/visitor.h"

class TypeChecker : public Visitor {
public:
    TypeChecker();
    ~TypeChecker() override;
};

#include <bits/stdc++.h>
#include <iostream>
#include "h1.h"

constexpr int VALUE = 114514;

struct StructRetain {};
struct StructRemove {};

template <class T = StructRetain, int N = VALUE>
struct TemplateStructRetain {};
template <class T, int N>
struct TemplateStructRemove {};

template <class T, int N>
struct TemplateStructRetain<T*, N> {};
template <class T, int N>
struct TemplateStructRemove<T*, N> {};

int global_value_retain = 0;
int global_value_remove = 1;

namespace Retain
{
    int space_value = 1;
} // namespace retain
namespace Remove
{
    int space_value_ = 1;
} // namespace remove

using Retain::space_value, Remove::space_value_;

using TypeAliasRetain = StructRetain;
using TypeAliasRemove = StructRemove;

template <class T, int N>
using TemplateTypeAliasRetain = TemplateStructRetain<T, N>;
template <class T, int N>
using TemplateTypeAliasRemove = TemplateStructRemove<T, N>;

enum EnumRetain { A, B };
enum EnumRemove { C, D };

enum class ClassEnumRetain { AA, BB };
enum class ClassEnumRemove { CC, DD };

void FunctionRetain() {}
void FunctionRemove() {}

template <class T>
void TemplateFunctionRetain() {}
template <class T>
void TemplateFunctionRemove() {}

template <>
void TemplateFunctionRetain<int>() {
    space_value = 000;
}
template <>
void TemplateFunctionRemove<int>() {
    space_value = 000;
}

template <int N>
constexpr auto template_value_retain = N;
template <int N>
constexpr auto template_value_remove = N;

template  <>
int template_value_retain<11> = 20;

int main() {
    H h;
    StructRetain a;
    TemplateStructRetain<int, 0> b;
    global_value_retain = 1;
    global_value_retain = A;
    Retain::space_value = 2;
    space_value = 2;

    space_value = static_cast<int>(ClassEnumRetain::AA);
    TypeAliasRetain c;
    TemplateTypeAliasRetain<int, 0> d;

    FunctionRetain();

    TemplateFunctionRetain<int>();

    global_value_retain = template_value_retain<10>;
}
/* MIT License
 *
 * Copyright (c) 2019 Gianluca Martino
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#pragma once

#include <unordered_map>
#include <typeinfo>
#include <typeindex>
#include <functional>
#include <variant>
#include <any>
#include <cassert>

template <typename ReturnType>
class multi_signature_callable
{
  std::variant<std::function<ReturnType()>, std::function<ReturnType(ReturnType)>, std::function<ReturnType(ReturnType, ReturnType)>> callable;

public:

//  multi_signature_callable<ReturnType>& operator=(std::function<ReturnType()> f) {
//    callable = f;
//    return *this;
//  }
//
//  multi_signature_callable<ReturnType>& operator=(std::function<ReturnType(ReturnType)> f) {
//    callable = f;
//    return *this;
//  }
//
//  multi_signature_callable<ReturnType>& operator=(std::function<ReturnType(ReturnType, ReturnType)> f) {
//    callable = f;
//    return *this;
//  }

  multi_signature_callable<ReturnType>& operator=(std::variant<std::function<ReturnType()>, std::function<ReturnType(ReturnType)>, std::function<ReturnType(ReturnType, ReturnType)>> c) {
    if (std::holds_alternative<std::function<ReturnType()>>(c)) {
      callable = std::get<std::function<ReturnType()>>(c);
    }
    else if (std::holds_alternative<std::function<ReturnType(ReturnType)>>(c)) {
      callable = std::get<std::function<ReturnType(ReturnType)>>(c);
    }
    else if (std::holds_alternative<std::function<ReturnType(ReturnType, ReturnType)>>(c)) {
      callable = std::get<std::function<ReturnType(ReturnType, ReturnType)>>(c);
    }
    else throw std::runtime_error("Something is very wrong in the assignment of the variant");
    return *this;
  }

  ReturnType operator()() const
  {
    assert(std::holds_alternative<std::function<ReturnType()>>(callable));
    auto f = std::get<std::function<ReturnType()>>(callable);
    return f();
  }

  ReturnType operator()(ReturnType parameter) const
  {
    assert(std::holds_alternative<std::function<ReturnType(ReturnType)>>(callable));
    auto f = std::get<std::function<ReturnType(ReturnType)>>(callable);
    return f(parameter);
  }

  ReturnType operator()(ReturnType parameter1, ReturnType parameter2) const
  {
    assert(std::holds_alternative<std::function<ReturnType(ReturnType, ReturnType)>>(callable));
    auto f = std::get<std::function<ReturnType(ReturnType, ReturnType)>>(callable);
    return f(parameter1, parameter2);
  }

};

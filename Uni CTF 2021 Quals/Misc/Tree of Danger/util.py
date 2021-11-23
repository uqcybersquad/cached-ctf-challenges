#!/usr/bin/env python3.10

import ast
import math
from typing import Union


def is_expression_safe(node: Union[ast.Expression, ast.AST]) -> bool:
    match type(node):
        case ast.Constant:
            return True
        case ast.List | ast.Tuple | ast.Set:
            return is_sequence_safe(node)
        case ast.Dict:
            return is_dict_safe(node)
        case ast.Name:
            return node.id == "math" and isinstance(node.ctx, ast.Load)
        case ast.UnaryOp:
            return is_expression_safe(node.operand)
        case ast.BinOp:
            return is_expression_safe(node.left) and is_expression_safe(node.right)
        case ast.Call:
            return is_call_safe(node)
        case ast.Attribute:
            return is_expression_safe(node.value)
        case _:
            return False


def is_sequence_safe(node: Union[ast.List, ast.Tuple, ast.Set]):
    return all(map(is_expression_safe, node.elts))


def is_dict_safe(node: ast.Dict) -> bool:
    for k, v in zip(node.keys, node.values):
        if not is_expression_safe(k) and is_expression_safe(v):
            return False
    return True


def is_call_safe(node: ast.Call) -> bool:
    if not is_expression_safe(node.func):
        return False
    if not all(map(is_expression_safe, node.args)):
        return False
    if node.keywords:
        return False
    return True


def is_safe(expr: str) -> bool:
    for bad in ['_']:
        if bad in expr:
            # Just in case!
            return False
    return is_expression_safe(ast.parse(expr, mode='eval').body)


if __name__ == "__main__":
    print("Welcome to SafetyCalc (tm)!\n"
          "Note: SafetyCorp are not liable for any accidents that may occur while using SafetyCalc")
    while True:
        ex = input("> ")
        if is_safe(ex):
            try:
                print(eval(ex, {'math': math}, {}))
            except Exception as e:
                print(f"Something bad happened! {e}")
        else:
            print("Unsafe command detected! The snake approaches...")
            exit(-1)

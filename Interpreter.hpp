//
// Created by stein on 2/17/2023.
//
#ifndef BOOLEANLOGICINTERPRETER_INTERPRETER_HPP
#define BOOLEANLOGICINTERPRETER_INTERPRETER_HPP

#include <iostream>
#include <string>
#include <stack>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <algorithm>

#define EXIT          (-1)
#define T             "T"
#define F             "F"
#define LEFT_BRACKET  "("
#define RIGHT_BRACKET ")"
#define VAR           "let"
#define OR            "|"
#define AND           "&"
#define NOT           "~"
#define EQ            "="

struct Node {
    enum Type {
        NUMBER, VARIABLE, OPERATOR
    };

    Type type;
    std::string value;
    Node *left, *right;

    Node(Type type, std::string value, Node* left = nullptr, Node* right = nullptr) :
            type(type), value(std::move(value)), left(left), right(right) {}
};

class Interpreter {
public:
    Interpreter() = default;

    void run() {
        bool running = true;
        do {
            std::string line;
            std::cout << "&> ";
            if (!std::getline(std::cin, line))
                break;
            if (line.empty())
                break;
            try {
                int res = execute(line);
                if (res == EXIT)
                    running = false;
            } catch (const std::exception& e) {
                std::cerr << "Error: " << e.what() << std::endl;
            }
        } while (running);
    }

    int execute(const std::string& line) {
        Node* tree = generate_tree(line);
//        std::cout << std::endl;
//        print_tree(tree); // Debugging
//        std::cout << std::endl;

        std::string res = evaluate_tree(tree);
        std::cout << res << std::endl;
        return 0;
    }

private:
    std::unordered_map<std::string, std::string> symbol_table;

    Node* generate_tree(const std::string& line) {
        std::vector<std::string> tokens = tokenize(line);
        std::stack<Node*> nodes;
        std::stack<std::string> ops;
        for (const std::string& token : tokens) {
            if (is_sign(token))
                nodes.push(new Node(Node::NUMBER, token));
            else if (is_variable(token))
                nodes.push(new Node(Node::VARIABLE, token));
            else if (is_operator(token)) {
                while (!ops.empty() && is_operator(ops.top()) && get_precedence(ops.top()) >= get_precedence(token)) {
                    Node* rhs = nullptr;
                    if (ops.top() != NOT) {
                        rhs = nodes.empty() ? nullptr : nodes.top(); if (!nodes.empty()) nodes.pop();
                    }
                    Node* lhs = nodes.empty() ? nullptr : nodes.top(); if (!nodes.empty()) nodes.pop();
                    nodes.push(new Node(Node::OPERATOR, ops.top(), lhs, rhs));
                    ops.pop();
                }
                ops.push(token);
            } else if (token == LEFT_BRACKET) {
                ops.push(token);
            } else if (token == RIGHT_BRACKET) {
                while (!ops.empty() && ops.top() != LEFT_BRACKET) {
                    Node* rhs = nodes.empty() ? nullptr : nodes.top(); if (!nodes.empty()) nodes.pop();
                    Node* lhs = nodes.empty() ? nullptr : nodes.top(); if (!nodes.empty()) nodes.pop();
                    nodes.push(new Node(Node::OPERATOR, ops.top(), lhs, rhs));
                    ops.pop();
                }
                ops.pop();
            }
        }
        while (!ops.empty()) {
            Node* rhs = nullptr;
            if (ops.top() != NOT) {
                rhs = nodes.empty() ? nullptr : nodes.top(); if (!nodes.empty()) nodes.pop();
            }
            Node* lhs = nodes.empty() ? nullptr : nodes.top(); if (!nodes.empty()) nodes.pop();
            nodes.push(new Node(Node::OPERATOR, ops.top(), lhs, rhs));
            ops.pop();
        }
        return nodes.top();
    }

    std::string evaluate_tree(Node* node) {
        if (node == nullptr) {
            return F;
        }
        switch (node->type) {
            case Node::NUMBER:
                return node->value;
            case Node::VARIABLE:
                if (is_variable_in_program(node->value)) {
                    return symbol_table[node->value];
                }
                throw std::runtime_error("Undefined variable: '" + node->value + "'");

            case Node::OPERATOR:
                if (node->value == EQ) {
                    if (node->left && node->left->type == Node::VARIABLE) {
                        if (!is_variable_in_program(node->left->value)) {
                            std::string rhs = evaluate_tree(node->right);
                            symbol_table[node->left->value] = rhs;
                            std::cout << node->left->value << ": ";
                            return rhs;
                        }
                    }
                    return evaluate_tree(node->left) == evaluate_tree(node->right) ? T : F;
                }

                std::string lhs = evaluate_tree(node->left);
                std::string rhs = evaluate_tree(node->right);

                if (node->value == NOT) {
                    return lhs == T ? F : T;
                }

                if (node->value == AND) {
                    return (rhs == T && lhs == T) ? T : F;
                }

                if (node->value == OR) {
                    return (rhs == T || lhs == T) ? T : F;
                }

                throw std::runtime_error("Unknown operator: '" + node->value + "'");
        }
        return T;
    }

    void print_tree(const Node* node) {
        print_tree("", node, false);
    }

    void print_tree(const std::string& prefix, const Node* node, bool is_left) const {
        if (node == nullptr)
            return;
        std::cout << prefix;
        std::cout << (is_left ? "├──" : "└──" );
        std::cout << node->value << std::endl;
        std::string suffix = (is_left ? "│   " : "    ");
        print_tree(prefix + suffix, node->left, true);
        print_tree(prefix + suffix, node->right, false);
    }

    std::vector<std::string> tokenize(const std::string& s) const {
        std::vector<std::string> tokens;
        int sz = (int) s.size();
        for (int i = 0; i < sz; ) {
            if (std::isspace(s[i]))
                i++;
            else if (i + 4 < sz && s.substr(i, 3) == VAR) {
                i += 3;
            }
            else if (std::isalpha(s[i])) {
                int j = i;
                while (j < sz && std::isalnum(s[j]))
                    j++;
                tokens.push_back(s.substr(i, j - i));
                i = j;
            } else {
                tokens.emplace_back(1, s[i]);
                i++;
            }
        }
        return tokens;
    }

    bool is_sign(const std::string& si) const {
        return si == T || si == F;
    }

    bool is_variable(const std::string& si) const {
        auto it = std::find_if_not(si.begin(), si.end(), (int(*)(int)) std::isalnum);
        return std::isalpha(si[0]) && it == si.end();
    }

    bool is_variable_in_program(const std::string& si) const {
        return is_variable(si) && symbol_table.count(si);
    }

    bool is_operator(const std::string& si) const {
        const std::unordered_set<std::string> ops = {
                AND, OR, EQ, NOT
        };
        return ops.count(si) > 0;
    }

    int get_precedence(const std::string& op) {
        if (op == OR)
            return 1;
        if (op == AND)
            return 2;
        if (op == EQ)
            return 3;
        if (op == NOT)
            return 4;
        if (op == LEFT_BRACKET || op == RIGHT_BRACKET)
            return 5;
        return 0;
    }
};
#endif //BOOLEANLOGICINTERPRETER_INTERPRETER_HPP

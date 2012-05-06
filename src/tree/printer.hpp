#pragma once
#include "../common.hpp"
#include "node.hpp"

namespace Mirb
{
	class Symbol;

	namespace Tree
	{
		struct MultipleExpressionNode;
		struct CaseEntry;
	};
	
	class Printer
	{
		protected:
			virtual std::string node(Tree::SimpleNode *node, size_t indent);
			virtual std::string wrap(Tree::SimpleNode *node, std::string result);
			template<class T> std::string join(T &list, std::string seperator = "");
			template<class T> std::string join(T &list, std::string pre, std::string post);
			std::string print_node(Tree::SimpleNode *node, size_t indent);
			std::string print_node(Tree::MultipleExpressionNode *node);
			std::string print_node(Tree::CaseEntry *node);
			std::string print_symbol(Symbol *symbol);
			virtual std::string get_indent(size_t indent);
		public:
			std::string print_node(Tree::SimpleNode *node);
	};
	
	class DebugPrinter:
		public Printer
	{
		protected:
			virtual std::string wrap(Tree::SimpleNode *node, std::string result);
	};
};

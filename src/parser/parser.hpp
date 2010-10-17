#pragma once

#include "../../globals.hpp"
#include "../../runtime/classes.hpp"
#include "../../runtime/runtime.hpp"
#include "../../compiler/lexer.hpp"
#include "../../compiler/ast.hpp"
#include "../../compiler/block.hpp"
#include "../../compiler/compiler.hpp"

#include "../lexer/lexer.hpp"
#include "../tree/nodes.hpp"

namespace Mirb
{
	class Compiler;
	
	class Parser
	{
		public:
			Parser(SymbolPool &symbol_pool, MemoryPool &memory_pool, Compiler &compiler);
			~Parser();
			
			Lexer lexer;
			MemoryPool &memory_pool;
			
			struct block *scope_defined(struct block *block, rt_value name, bool recursive);
			struct variable *scope_declare_var(struct block *block, rt_value name);
			struct variable *scope_var(struct block *block);
			struct variable *scope_define(struct block *block, rt_value name);
			struct variable *scope_get(struct block *block, rt_value name);
			
			void unexpected(bool skip = true);
			void expected(Lexeme::Type what, bool skip = false);
			
			void error(std::string text);
			
			struct compiler *old_compiler;
			struct block *current_block;
						
			static size_t operator_precedences[];

			bool is_precedence_operator(Lexeme::Type op);
			Node *parse_precedence_operator();
			Node *parse_precedence_operator(Node *left, size_t min_precedence);
			
			// expressions
			bool is_assignment_op();
			bool is_equality_op();
			bool is_sep();
			void skip_seps();
			Node *parse_assignment(VariableNode *variable);
			Node *parse_array();
			Node *parse_identifier();
			Node *parse_unary();
			Node *parse_boolean_unary();
			Node *parse_factor();
			void parse_arguments(NodeList &arguments);
			Node *parse_boolean();
			
			Node *parse_expression()
			{
				return parse_ternary_if();
			}
			
			Node *parse_statement()
			{
				return parse_conditional();
			}
			
			Node *parse_group();
			
			void parse_sep();
			void parse_statements(NodeList &list);
			
			void parse_main(Scope &scope);
			
			// control flow
			Node *parse_if_tail();
			void parse_then_sep();
			Node *parse_if();
			Node *parse_unless();
			Node *parse_ternary_if();
			Node *parse_conditional();
			Node *parse_case();
			Node *parse_begin();
			Node *parse_exception_handlers(Node *block);
			Node *parse_return();
			Node *parse_break();
			Node *parse_next();
			Node *parse_redo();
			
			// calls
			bool require_ident();
			bool has_arguments();
			bool is_lookup();
			Node *parse_lookup(Node *child);
			Node *parse_lookup_tail(Node *tail);
			BlockNode *parse_block();
			void parse_arguments(NodeList &arguments, bool has_args, bool *parenthesis);
			Node *alloc_call_node(Node *object, Symbol *symbol, bool has_args);
			Node *secure_block(BlockNode *result, Node *parent);
			Node *parse_call(Symbol *symbol, Node *child, bool default_var);
			Node *parse_lookup_chain();
			Node *parse_yield();
			Node *parse_super();
			
			// structures
			bool is_parameter();
			void parse_parameters(struct block *block);
			Node *parse_class();
			Node *parse_module();
			Node *parse_method();
			
			
			Lexeme::Type lexeme()
			{
				return lexer.lexeme.type;
			}
			
		private:
			Compiler &compiler;
			
			bool match(Lexeme::Type what)
			{
				if(lexeme() == what)
				{
					lexer.step();
					return true;
				}
				else
				{
					expected(what);
					return false;
				}
			}
			
			bool require(Lexeme::Type what)
			{
				if(lexeme() == what)
				{
					return true;
				}
				else
				{
					expected(what);
					return false;
				}
			}
			
			bool matches(Lexeme::Type what)
			{
				if(lexeme() == what)
				{
					lexer.step();
					return true;
				}
				else
					return false;
			}
			
			bool expect(Lexeme::Type what)
			{
				if(lexeme() == what)
				{
					return true;
				}
				else
				{
					expected(what);
					return false;
				}
			}
			
			void skip_lines()
			{
				while(matches(Lexeme::LINE));
			}
			
			bool is_expression()
			{
				switch(lexeme())
				{
					case Lexeme::IDENT:
					case Lexeme::EXT_IDENT:
					case Lexeme::IVAR:
					case Lexeme::ADD:
					case Lexeme::SUB:
					case Lexeme::INTEGER:
					case Lexeme::KW_IF:
					case Lexeme::KW_UNLESS:
					case Lexeme::KW_CASE:
					case Lexeme::KW_BEGIN:
					case Lexeme::KW_CLASS:
					case Lexeme::KW_MODULE:
					case Lexeme::KW_NOT:
					case Lexeme::LOGICAL_NOT:
					case Lexeme::KW_DEF:
					case Lexeme::KW_SELF:
					case Lexeme::KW_TRUE:
					case Lexeme::KW_FALSE:
					case Lexeme::KW_NIL:
					case Lexeme::STRING:
					case Lexeme::STRING_START:
					case Lexeme::KW_YIELD:
					case Lexeme::KW_RETURN:
					case Lexeme::KW_BREAK:
					case Lexeme::KW_SUPER:
					case Lexeme::KW_REDO:
					case Lexeme::KW_NEXT:
					case Lexeme::PARENT_OPEN:
					case Lexeme::SQUARE_OPEN:
						return true;

					default:
						return false;
				}
			}
			
			struct node *alloc_node(enum node_type type)
			{
				struct node *result = new (memory_pool) struct node;
				
				result->type = type;
				result->unbalanced = false;

				return result;
			}
			
			struct block *alloc_scope(Scope &scope, enum block_type type);
	};
	
};

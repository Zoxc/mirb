#pragma once

#include "../../globals.hpp"
#include "../../runtime/classes.hpp"
#include "../../runtime/runtime.hpp"
#include "../../compiler/lexer.hpp"
#include "../../compiler/ast.hpp"
#include "../../compiler/block.hpp"
#include "../../compiler/compiler.hpp"

#include "../lexer/lexer.hpp"

namespace Mirb
{
	class Compiler;
	
	class Parser
	{
		public:
			Parser(SymbolPool &symbol_pool, MemoryPool &memory_pool, Compiler &compiler);
			~Parser();
			
			Lexer lexer;
			
			void unexpected(bool skip = true);
			void expected(Lexeme::Type what, bool skip = false);
			
			void error(std::string text);
			
			struct compiler *old_compiler;
			struct block *current_block;
			
			// expressions
			bool is_equality_op();
			bool is_sep();
			void skip_seps();
			struct node *parse_array_element();
			struct node *parse_identifier();
			struct node *parse_unary();
			struct node *parse_term();
			struct node *parse_boolean_and();
			struct node *parse_low_boolean_unary();
			struct node *parse_equality();
			struct node *parse_shift();
			struct node *parse_factor();
			struct node *parse_expression();
			struct node *parse_argument();
			struct node *parse_arithmetic();
			struct node *parse_boolean_or();
			struct node *parse_low_boolean();
			struct node *parse_statement();
			struct node *parse_statements();
			void parse_sep();
			struct node *parse_main();
			
			// control flow
			struct node *parse_if_tail();
			void parse_then_sep();
			struct node *parse_case_body();
			struct node *parse_if();
			struct node *parse_unless();
			struct node *parse_ternary_if();
			struct node *parse_conditional();
			struct node *parse_case();
			struct node *parse_begin();
			struct node *parse_exception_handlers(struct node *block);
			struct node *parse_return();
			struct node *parse_break();
			struct node *parse_next();
			struct node *parse_redo();
			
			// calls
			bool require_ident();
			bool has_arguments();
			bool is_lookup();
			struct node *parse_lookup(struct node *child);
			struct node *parse_lookup_tail(struct node *tail);
			struct node *parse_block();
			struct node *parse_arguments(bool has_args, bool *parenthesis);
			struct node *alloc_call_node(struct node *child, rt_value symbol, bool has_args);
			struct node *secure_block(struct node *result, struct node *parent);
			struct node *parse_call(rt_value symbol, struct node *child, bool default_var);
			struct node *parse_lookup_chain();
			struct node *parse_yield();
			struct node *parse_super();
			
			// structures
			bool is_parameter();
			void parse_parameter(struct block *block);
			struct node *parse_class();
			struct node *parse_module();
			struct node *parse_method();
			
			
			Lexeme::Type lexeme()
			{
				return lexer.lexeme.type;
			}
			
		private:
			Compiler &compiler;
			MemoryPool &memory_pool;
			
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
			
			struct node *alloc_scope(struct block **block_var, enum block_type type);
			
			struct block *scope_defined(struct block *block, rt_value name, bool recursive);
			struct variable *scope_declare_var(struct block *block, rt_value name);
			struct variable *scope_var(struct block *block);
			struct variable *scope_define(struct block *block, rt_value name);
			struct variable *scope_get(struct block *block, rt_value name);
			
			#define ASSIGN_OPS \
					case Lexeme::ASSIGN_ADD: \
					case Lexeme::ASSIGN_SUB: \
					case Lexeme::ASSIGN_MUL: \
					case Lexeme::ASSIGN_DIV: \
					case Lexeme::ASSIGN_LEFT_SHIFT: \
					case Lexeme::ASSIGN_RIGHT_SHIFT:
			
			/*
			 * These nodes uses no arguments and can be statically allocated.
			 */
			static struct node nil_node;
			static struct node self_node;
			static struct node true_node;
			static struct node false_node;
	};
	
};

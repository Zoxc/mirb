#include "parser.hpp"
#include "../compiler.hpp"
#include "../document.hpp"

namespace Mirb
{
	Parser::Parser(SymbolPool &symbol_pool, MemoryPool memory_pool, Document *document) :
		lexer(symbol_pool, memory_pool, *this),
		document(*document),
		memory_pool(memory_pool),
		fragment(nullptr),
		scope(nullptr),
		trapper(nullptr)
	{
	}
	
	Parser::~Parser()
	{
		Message *current = messages.first;

		while(current)
		{
			Message *next = current->entry.next;

			current->~Message();

			current = next;
		}
	}

	SourceLoc *Parser::capture()
	{
		return new (fragment) SourceLoc(lexer.lexeme);
	}
	
	void Parser::load()
	{
		mirb_debug_assert(document.data[document.length] == 0);

		lexer.load(document.data, document.length);
	}
	
	void Parser::report(const SourceLoc &range, std::string text, Message::Severity severity)
	{
		add_message(new (memory_pool) Message(*this, range, severity, text), range);
	}

	bool Parser::is_constant(Symbol *symbol)
	{
		const char_t c = *symbol->string.raw();
		
		return c >= 'A' && c <= 'Z';
	}
	
	void Parser::error(std::string text)
	{
		report(lexer.lexeme, text);
	}

	void Parser::expected(Lexeme::Type what, bool skip)
	{
		error("Expected " + Lexeme::describe_type(what) + ", but found " + lexer.lexeme.describe());
		
		if(skip)
			lexer.step();
	}
	
	void Parser::unexpected(bool skip)
	{
		error("Unexpected " + lexer.lexeme.describe());

		if(skip)
			lexer.step();
	}

	void Parser::add_message(Message *message, const SourceLoc &range)
	{
		auto i = messages.mutable_iterator();
		
		while(i)
		{
			if(i().range.start > range.start)
				break;
				
			++i;
		}
		
		i.insert(message);
	}

	bool Parser::close_pair(const std::string &name, const SourceLoc &range, Lexeme::Type what, bool skip)
	{
		if(lexeme() == what)
		{
			if(skip)
				lexer.step();

			return true;
		}
		else
		{
			auto message = new (memory_pool) Message(*this, range, Message::MESSAGE_ERROR, "Unterminated " + name);
			
			message->note = new (memory_pool) Message(*this, lexer.lexeme, Message::MESSAGE_NOTE, "Expected " + Lexeme::describe_type(what) + " here, but found " + lexer.lexeme.describe());
			
			add_message(message, lexer.lexeme);

			return false;
		}
	}

	void Parser::parse_sep()
	{
		switch (lexeme())
		{
			case Lexeme::LINE:
			case Lexeme::SEMICOLON:
				lexer.step();
				break;

			default:
				expected(Lexeme::SEMICOLON);
				break;
		}
	}

	SourceLoc *Parser::parse_method_name(Symbol *&symbol)
	{
		SourceLoc *range = capture();

		switch(lexeme())
		{
			case Lexeme::IDENT:
			{
				symbol = lexer.lexeme.symbol;

				lexer.step();

				if(lexeme() == Lexeme::ASSIGN && lexer.lexeme.whitespace == false)
				{
					symbol = symbol_pool.get(symbol->string + "=");

					range->expand(lexer.lexeme);

					lexer.step();
				}

				return range;
			}
			
			case Lexeme::EXT_IDENT:
			{
				symbol = lexer.lexeme.symbol;

				lexer.step();

				return range;
			}
			
			case Lexeme::POWER:
			case Lexeme::MUL:
			case Lexeme::DIV:
			case Lexeme::MOD:
			case Lexeme::ADD:
			case Lexeme::SUB:
			case Lexeme::LEFT_SHIFT:
			case Lexeme::RIGHT_SHIFT:
			case Lexeme::AMPERSAND:
			case Lexeme::BITWISE_XOR:
			case Lexeme::BITWISE_OR:
			case Lexeme::COMPARE:
			case Lexeme::LESS:
			case Lexeme::LESS_OR_EQUAL:
			case Lexeme::GREATER:
			case Lexeme::GREATER_OR_EQUAL:
			case Lexeme::EQUALITY:
			case Lexeme::NO_EQUALITY:
			case Lexeme::CASE_EQUALITY:
			case Lexeme::MATCHES:
			case Lexeme::NOT_MATCHES:
			case Lexeme::BITWISE_NOT:
			case Lexeme::LOGICAL_NOT:
			case Lexeme::UNARY_ADD:
			case Lexeme::UNARY_SUB:
			case Lexeme::BACKTICK:
			{
				symbol = symbol_pool.get(lexer.lexeme);

				lexer.step();

				return range;

			}

			case Lexeme::SQUARE_OPEN:
			{
				lexer.step();

				if(lexer.lexeme.whitespace == false && match(Lexeme::SQUARE_CLOSE))
				{
					if(lexer.lexeme.whitespace == false && lexeme() == Lexeme::ASSIGN)
					{
						symbol = symbol_pool.get("[]=");

						lexer.step();
					}
					else
					{
						symbol = symbol_pool.get("[]");
					}

					lexer.lexeme.prev_set(range);
				}
				else
					symbol = 0;

				return range;
			}
			
			default:
			{
				expected(Lexeme::IDENT);
					
				return 0;
			}
		}
	}
				
	Tree::Node *Parser::parse_variable(Symbol *symbol, SourceLoc *range)
	{
		if(is_constant(symbol))
		{
			return new (fragment) Tree::ConstantNode(nullptr, symbol, range);
		}
		else
		{
			auto result = new (fragment) Tree::VariableNode;
			
			result->var = scope->get_var<Tree::NamedVariable>(symbol);
			
			return result;
		}
	}
	
	void Parser::parse_argument_list(Tree::InvokeNode *node, Lexeme::Type terminator)
	{
		Tree::HashNode *hash = nullptr;
		SourceLoc last_hash;

		do
		{
			skip_lines();
			
			if(terminator != Lexeme::NONE && lexeme() == terminator)
				return;

			if(lexeme() == Lexeme::AMPERSAND)
			{
				SourceLoc range = lexer.lexeme;
				lexer.step();
				auto result = parse_splat_operator_expression();
				lexer.lexeme.prev_set(&range);

				if(node->block_arg)
					report(range, "There can only be a single block argument");
				else
					node->block_arg = new (fragment) Tree::BlockArg(range, result);
			}
			else
			{
				if(node->block_arg)
					report(node->block_arg->range, "The block argument must be the last one");

				SourceLoc range = lexer.lexeme;

				auto result = parse_splat_operator_expression();

				if(result)
				{
					if(result->type() == Tree::Node::Splat)
						node->variadic = true;

					parse_association_argument(result, range, last_hash, hash, [&](Tree::Node *child) { node->arguments.append(child); });
				}
			}
		}
		while(matches(Lexeme::COMMA));
	}
	
	bool Parser::is_assignment_op()
	{
		switch (lexeme())
		{
			case Lexeme::ASSIGN_POWER:
			case Lexeme::ASSIGN_ADD:
			case Lexeme::ASSIGN_SUB:
			case Lexeme::ASSIGN_MUL:
			case Lexeme::ASSIGN_DIV:
			case Lexeme::ASSIGN_MOD:
			case Lexeme::ASSIGN_LEFT_SHIFT:
			case Lexeme::ASSIGN_RIGHT_SHIFT:
			case Lexeme::ASSIGN_BITWISE_AND:
			case Lexeme::ASSIGN_BITWISE_XOR:
			case Lexeme::ASSIGN_BITWISE_OR:
			case Lexeme::ASSIGN_LOGICAL_AND:
			case Lexeme::ASSIGN_LOGICAL_OR:
			case Lexeme::ASSIGN:
				return true;
			
			default:
				return false;
		}
	}
	
	Tree::Node *Parser::parse_hash()
	{
		auto result = new (fragment) Tree::HashNode;

		SourceLoc range = lexer.lexeme;
		
		lexer.step();
		
		do
		{
			skip_lines();
				
			if(lexeme() == Lexeme::CURLY_CLOSE)
				break;

			auto range = capture();
			auto key = parse_operator_expression(false);
			lexer.lexeme.prev_set(range);

			if(lexeme() == Lexeme::COLON)
			{
				auto node = new (fragment) Tree::SymbolNode;
				auto call = static_cast<Tree::CallNode *>(key);

				if(key->type() == Tree::Node::Variable)
					node->symbol = static_cast<Tree::VariableNode *>(key)->var->name;
				else if(key->type() == Tree::Node::Call && call->can_be_var)
					node->symbol = call->method;
				else
					error("Use '=>' to associate non-identifier key");

				result->entries.append(node);

				lexer.step();
				skip_lines();
			}
			else
			{
				if(key)
					result->entries.append(key);

				match(Lexeme::ASSOC);
				skip_lines();
			}

			auto value = parse_operator_expression(false);
				
			if(value)
				result->entries.append(value);
		}
		while(matches(Lexeme::COMMA));

		skip_lines();

		close_pair("hash literal", range, Lexeme::CURLY_CLOSE);

		lexer.lexeme.prev_set(&range);

		result->range = range;

		return result;
	}
	
	Tree::Node *Parser::parse_array()
	{
		auto result = new (fragment) Tree::ArrayNode;

		result->variadic = false;
		
		SourceLoc range = lexer.lexeme;

		Tree::HashNode *hash = nullptr;
		SourceLoc last_hash;
		
		lexer.step();

		do
		{
			skip_lines();
				
			if(lexeme() == Lexeme::SQUARE_CLOSE)
				break;

			SourceLoc element_range = lexer.lexeme;

			auto element = parse_splat_operator_expression();

			lexer.lexeme.prev_set(&element_range);
				
			if(element)
			{
				if(element->type() == Tree::Node::Splat)
					result->variadic = true;
				
				parse_association_argument(element, element_range, last_hash, hash, [&](Tree::Node *child) { result->entries.append(child); });
			}
		}
		while(matches(Lexeme::COMMA));

		skip_lines();		

		close_pair("array literal", range, Lexeme::SQUARE_CLOSE);

		return result;
	}

	Tree::Node *Parser::parse_data(Lexeme::Type override)
	{
		Value::Type type;

		switch(override)
		{
			case Lexeme::COMMAND:
			{
				SourceLoc range = lexer.lexeme;

				auto command = parse_data(Lexeme::STRING);

				auto result = new (fragment) Tree::CallNode;

				if(command)
					result->arguments.append(command);

				lexer.lexeme.prev_set(&range);

				result->object = new (fragment) Tree::SelfNode;
				result->method = Symbol::get("`");
				result->range = new (fragment) SourceLoc(range);

				return result;
			}
			case Lexeme::STRING:
				type = Value::String;
				break;
				
			case Lexeme::REGEXP:
				type = Value::Regexp;
				break;
				
			case Lexeme::SYMBOL:
				type = Value::Symbol;
				break;

			case Lexeme::ARRAY:
				type = Value::Array;
				break;

			default:
				mirb_debug_abort("Unknown token");
		}

		switch(lexer.lexeme.data->type)
		{
			case InterpolateData::Plain:
			{
				auto data = lexer.lexeme.data;

				if(data->entries.size())
				{
					auto result = new (fragment) Tree::InterpolateNode;

					result->result_type = type;

					process_interpolate_entries(result, result->data);

					result->data = data->tail.copy<Tree::Fragment>(fragment);

					lexer.step();

					return result;
				}
				else
				{
					auto result = new (fragment) Tree::DataNode;
				
					result->result_type = type;
					result->range = lexer.lexeme;

					result->data = data->tail.copy<Tree::Fragment>(fragment);
					
					lexer.step();

					return result;
				}
			}

			case InterpolateData::Starting:
			{
				Lexeme::Type token = lexeme();

				auto result = new (fragment) Tree::InterpolateNode;

				SourceLoc range = lexer.lexeme;
				
				result->result_type = type;

				do
				{
					auto pair = new (fragment) Tree::InterpolatePairNode;

					process_interpolate_entries(result, pair->string);

					lexer.step();
					
					pair->group = parse_group();
					
					result->pairs.append(pair);

					skip_lines();
				}
				while(lexeme() == token && lexer.lexeme.data->type == InterpolateData::Continuing);

				if(lexeme() == token && lexer.lexeme.data->type == InterpolateData::Ending)
				{
					process_interpolate_entries(result, result->data);

					result->data = lexer.lexeme.data->tail.copy<Tree::Fragment>(fragment);

					lexer.step();
				}
				else
				{
					auto message = new (memory_pool) Message(*this, range, Message::MESSAGE_ERROR, "Unterminated interpolate sequence of " +  Lexeme::describe_type(token));
			
					message->note = new (memory_pool) Message(*this, lexer.lexeme, Message::MESSAGE_NOTE, "Expected terminating " + Lexeme::describe_type(token) + " here, but found " + lexer.lexeme.describe());
			
					add_message(message, lexer.lexeme);
				}

				return result;
			}
			
			default:
			{
				unexpected();
				return nullptr;
			}
		}
	}

	void Parser::process_interpolate_entries(Tree::InterpolateNode *root, InterpolateData::Entry &tail)
	{
		auto &entries = lexer.lexeme.data->entries;

		for(auto entry: entries)
		{
			auto pair = new (fragment) Tree::InterpolatePairNode;
			pair->string = entry->copy<Tree::Fragment>(fragment);

			switch(entry->type)
			{
				case Lexeme::CVAR:
					pair->group = new (fragment) Tree::CVarNode(entry->symbol);
					break;

				case Lexeme::IVAR:
					pair->group = new (fragment) Tree::IVarNode(entry->symbol);
					break;

				case Lexeme::GLOBAL:
					pair->group = new (fragment) Tree::GlobalNode(entry->symbol);
					break;

				default:
					mirb_debug_abort("Unknown string type");
			}

			root->pairs.append(pair);
		}

		tail = lexer.lexeme.data->tail.copy<Tree::Fragment>(fragment);
	}
	
	Tree::Node *Parser::parse_symbol()
	{
		switch(lexeme())
		{
			case Lexeme::COLON:
			{
				SourceLoc range = lexer.lexeme;

				lexer.lexeme.allow_keywords = false;

				lexer.step();

				lexer.lexeme.allow_keywords = true;

				if(lexer.lexeme.whitespace)
				{
					range.start = range.stop;
					range.stop = lexer.lexeme.start;

					report(range, "No whitespace between ':' and the literal is allowed with symbol literals");
				}

				switch(lexeme())
				{
					case Lexeme::IVAR:
					case Lexeme::CVAR:
					case Lexeme::GLOBAL:
					{
						auto result = new (fragment) Tree::SymbolNode;
						result->symbol = lexer.lexeme.symbol;
						lexer.step();
						return result;
					}
					
					case Lexeme::STRING:
						return parse_data(Lexeme::SYMBOL);

					default:
					{
						auto result = new (fragment) Tree::SymbolNode;

						parse_method_name(result->symbol);

						return result;
					}
				}

			}

			case Lexeme::SYMBOL:
			{
				auto result = new (fragment) Tree::SymbolNode;

				result->symbol = lexer.lexeme.symbol;

				lexer.step();

				return result;
			}
			
			default:
				expected(Lexeme::SYMBOL);
				return 0;
		}
	}

	Tree::Node *Parser::parse_factor()
	{
		switch (lexeme())
		{
			case Lexeme::KW_SPECIAL_FILE:
			{
				auto result = new (fragment) Tree::DataNode;

				result->result_type = Value::String;
				result->data.set<Tree::Fragment>(document.name.get_string(), fragment);

				lexer.step();

				return result;
			}
			
			case Lexeme::KW_DEFINED:
			{
				lexer.step();

				Tree::Node *node;

				SourceLoc range = lexer.lexeme;

				if(lexeme() == Lexeme::PARENT_OPEN)
				{
					lexer.step();

					node = parse_expression();

					close_pair("defined? parentheses", range, Lexeme::PARENT_CLOSE);
				}
				else
					node = parse_operator_expression();
				
				lexer.lexeme.prev_set(&range);

				switch(node->type())
				{
					case Tree::Node::Variable:
					{
						auto result = new (fragment) Tree::DataNode;
						result->result_type = Value::String;
						result->data.set<Tree::Fragment>("local-variable", fragment);
						return result;
					}

					case Tree::Node::Call:
					case Tree::Node::CVar:
					case Tree::Node::IVar:
					case Tree::Node::Global:
					case Tree::Node::Constant:
						break; // TODO: Implement

					default:
						break;
				};

				return new (fragment) Tree::NilNode;
			}

			case Lexeme::DIV:
			case Lexeme::ASSIGN_DIV:
			{
				lexer.div_to_regexp();

				return parse_data(lexeme());
			}
			
			case Lexeme::LEFT_SHIFT:
			{
				lexer.left_shift_to_heredoc();

				if(lexeme() == Lexeme::LEFT_SHIFT)
					break;
				else
				{
					auto result = lexer.lexeme.heredoc;

					lexer.step();

					return result;
				}
			}

			case Lexeme::MOD:
			case Lexeme::ASSIGN_MOD:
			{
				lexer.mod_to_literal();

				if(lexeme() == Lexeme::MOD)
					break;
				else
					return parse_data(lexeme());
			}

			case Lexeme::KW_BEGIN:
				return parse_begin();

			case Lexeme::KW_IF:
				return parse_if();

			case Lexeme::KW_UNLESS:
				return parse_unless();
				
			case Lexeme::KW_WHILE:
			case Lexeme::KW_UNTIL:
				return parse_loop();
				
			case Lexeme::KW_FOR:
				return parse_for_loop();

			case Lexeme::KW_CASE:
				return parse_case();

			case Lexeme::KW_CLASS:
				return parse_class();

			case Lexeme::KW_MODULE:
				return parse_module();

			case Lexeme::KW_DEF:
				return parse_method();

			case Lexeme::KW_YIELD:
				return parse_yield();

			case Lexeme::KW_RETURN:
				return parse_return();

			case Lexeme::KW_BREAK:
				return parse_break();

			case Lexeme::KW_NEXT:
				return parse_next();

			case Lexeme::KW_REDO:
				return parse_redo();

			case Lexeme::KW_SUPER:
				return parse_super();

			case Lexeme::SQUARE_OPEN:
				return parse_array();
				
			case Lexeme::CURLY_OPEN:
				return parse_hash();
				
			case Lexeme::QUESTION:
			{
				lexer.question_to_character();

				auto result = new (fragment) Tree::DataNode;
				
				result->result_type = Value::String;
					
				result->data = lexer.lexeme.data->tail.copy<Tree::Fragment>(fragment);
					
				lexer.step();

				return result;
			}
				
			case Lexeme::SYMBOL:
			case Lexeme::COLON:
				return parse_symbol();

			case Lexeme::COMMAND:
			case Lexeme::REGEXP:
			case Lexeme::ARRAY:
				return parse_data(lexeme());
			
			case Lexeme::STRING:
			{
				auto result = static_cast<Tree::DataNode *>(parse_data(lexeme()));
				
				if(!result)
					return 0;

				while(lexeme() == Lexeme::STRING && lexer.lexeme.data->beginning())
				{
					Tree::InterpolateNode *new_result;

					auto add = static_cast<Tree::DataNode *>(parse_data(lexeme()));

					if(!add)
						return 0;

					auto add_inter = static_cast<Tree::InterpolateNode *>(add);
					auto result_inter = static_cast<Tree::InterpolateNode *>(result);

					if(add->type() == Tree::Node::Interpolate)
					{
						new_result = new (fragment) Tree::InterpolateNode;
						new_result->data = result->data;
						new_result->result_type = Value::String;
					}
					else
						new_result = result_inter;

					if(add_inter->type() == Tree::Node::Interpolate && !add_inter->pairs.empty())
					{
						add_inter->pairs.first->string.set<Tree::Fragment>(result->data.get() + add_inter->pairs.first->string.get(), fragment);
						new_result->data = add_inter->data;

						Tree::InterpolatePairNode *current = add_inter->pairs.first;

						while(current)
						{
							auto next = current->entry.next;

							new_result->pairs.append(current);

							current = next;
						}
					}
					else
						new_result->data.add<Tree::Fragment>(add->data.get(), fragment);

					result = new_result;
				}

				return result;
			}

			case Lexeme::KW_SELF:
			{
				lexer.step();
				
				return new (fragment) Tree::SelfNode;
			}

			case Lexeme::KW_TRUE:
			{
				lexer.step();

				return new (fragment) Tree::TrueNode;
			}

			case Lexeme::KW_FALSE:
			{
				lexer.step();

				return new (fragment) Tree::FalseNode;
			}

			case Lexeme::KW_NIL:
			{
				lexer.step();

				return new (fragment) Tree::NilNode;
			}
			
			case Lexeme::REAL:
			{
				auto result = new (fragment) Tree::FloatNode;
					
				std::string str = lexer.lexeme.string();
					
				result->value = std::atof(str.c_str()); // TODO: Parse to double
					
				lexer.step();
					
				return result;
			}
			
			case Lexeme::BLOCK_POINT:
			{
				auto result = new (fragment) Tree::BlockNode;
				
				auto scope_range = capture();

				lexer.step();
				
				allocate_scope(Tree::Scope::Closure, [&] {
					result->scope = scope;

					if(lexeme() == Lexeme::PARENT_OPEN)
					{
						SourceLoc parent_open = lexer.lexeme;

						step_lines();

						if(lexeme() != Lexeme::PARENT_CLOSE)
							parse_parameters(false);

						close_pair("block parameters", parent_open, Lexeme::PARENT_CLOSE);
					}
				});

				lexer.lexeme.prev_set(scope_range);

				result->scope->range = scope_range;
				
				bool curly;

				SourceLoc range = lexer.lexeme;
		
				switch(lexeme())
				{
					case Lexeme::CURLY_OPEN:
						curly = true;
						break;
			
					case Lexeme::KW_DO:
						curly = false;
						break;
			
					default:
						return 0;
				}
		
				enter_scope(result->scope, [&] {
					step_lines();

					scope->group = parse_group();
				});
		
				close_pair("block", range, curly ? Lexeme::CURLY_CLOSE : Lexeme::KW_END);

				return result;
			}

			case Lexeme::BINARY:
			case Lexeme::OCTAL:
			case Lexeme::HEX:
			case Lexeme::INTEGER:
			{
				auto result = new (fragment) Tree::IntegerNode;
					
				std::string str = lexer.lexeme.string();
					
				result->value = std::atoi(str.c_str()); // TODO: Parse octal/hex/binary and longer numbers correctly
					
				lexer.step();
					
				return result;
			}
			
			case Lexeme::SCOPE:
			{
				auto range = capture();

				lexer.step();

				if(require(Lexeme::IDENT))
				{
					range->expand(lexer.lexeme);

					Symbol *symbol = lexer.lexeme.symbol;

					if(is_constant(symbol))
					{
						Tree::Node *result = new (fragment) Tree::ConstantNode(nullptr, symbol, range, true);

						lexer.step();

						while(is_lookup())
						{
							Tree::Node *node = parse_lookup(result);

							result = node;
						}

						return result;
					}
					else
					{
						Tree::Node *call_node = parse_call(0, new (fragment) Tree::SelfNode, 0, false); // Try to parse it as a method or local variable

						if(call_node->type() == Tree::Node::Call)
							range->expand(*((Tree::CallNode *)call_node)->range);

						report(*range, "Can only reference constants when leaving out the lookup expression (Use ::Object to access non-constants)");

						return call_node;
					}
				}
				
				return 0;
			}
			
			case Lexeme::CVAR:
			{
				Symbol *symbol = lexer.lexeme.symbol;
					
				lexer.step();
				
				return new (fragment) Tree::CVarNode(symbol);
			}

			case Lexeme::IVAR:
			{
				Symbol *symbol = lexer.lexeme.symbol;
					
				lexer.step();
				
				return new (fragment) Tree::IVarNode(symbol);
			}

			case Lexeme::GLOBAL:
			{
				Symbol *symbol = lexer.lexeme.symbol;
					
				lexer.step();
				
				return new (fragment) Tree::GlobalNode(symbol);
			}

			case Lexeme::IDENT:
			{
				Symbol *symbol = lexer.lexeme.symbol;
				auto range = capture();

				lexer.step();
				
				return parse_call(symbol, new (fragment) Tree::SelfNode, range, true); // Function call, constant or local variable
			}
			
			case Lexeme::UNARY_ADD:
			case Lexeme::UNARY_SUB:
			case Lexeme::EXT_IDENT:
			{
				auto symbol = symbol_pool.get(lexer.lexeme);
				auto range = capture();

				lexer.step();
				
				return parse_call(symbol, new (fragment) Tree::SelfNode, range, false);
			}

			case Lexeme::PARENT_OPEN:
			{
				SourceLoc range = lexer.lexeme;

				lexer.step();
				
				skip_lines();
					
				Tree::Node *result = parse_statements();

				close_pair("parentheses", range, Lexeme::PARENT_CLOSE);
				
				return result;
			}

			default:
				break;
		}

		error("Expected expression but found " + lexer.lexeme.describe());
		lexer.step();
		return 0;
	}
	
	Tree::Node *Parser::parse_power()
	{
		Tree::Node *result = parse_lookup_chain();
		
		if(lexeme() == Lexeme::POWER)
		{
			typecheck(result, [&](Tree::Node *result) -> Tree::Node * {
				auto node = new (fragment) Tree::BinaryOpNode;
			
				node->op = Lexeme::POWER;
				node->range = lexer.lexeme;
				node->left = result;
			
				lexer.step();
			
				node->right = typecheck(parse_unary());

				return node;
			});
		}
		
		return result;
	}

	Tree::Node *Parser::parse_unary()
	{
		switch(lexeme())
		{
			case Lexeme::BITWISE_NOT:
			case Lexeme::LOGICAL_NOT:
			case Lexeme::ADD:
			case Lexeme::SUB:
			{
				auto result = new (fragment) Tree::UnaryOpNode;

				result->range = capture();
				result->op = (lexeme() == Lexeme::BITWISE_NOT || lexeme() == Lexeme::LOGICAL_NOT) ? lexeme() : Lexeme::operator_to_unary(lexeme());

				lexer.step();

				result->value = typecheck(parse_unary());

				lexer.lexeme.prev_set(result->range);
				
				return result;
			}
			
			default:
				return parse_power();
		}
	}

	size_t Parser::operator_precedences[] = {
		8, // MUL,
		8, // DIV,
		8, // MOD,
		
		7, // ADD,
		7, // SUB,
		
		6, // LEFT_SHIFT,
		6, // RIGHT_SHIFT,
		
		5, // AMPERSAND, // BITWISE_AND
		
		4, // BITWISE_XOR,
		4, // BITWISE_OR,
		
		1, // LOGICAL_AND,
		
		0, // LOGICAL_OR,
		
		3, // GREATER,
		3, // GREATER_OR_EQUAL,
		3, // LESS,
		3, // LESS_OR_EQUAL,
		
		2, // COMPARE,
		2, // EQUALITY,
		2, // CASE_EQUALITY,
		2, // NO_EQUALITY,
		2, // MATCHES,
		2, // NOT_MATCHES,
	};

	bool Parser::is_precedence_operator(Lexeme::Type op)
	{
		return op >= Lexeme::precedence_operators_start && op <= Lexeme::precedence_operators_end;
	}

	Tree::Node *Parser::parse_precedence_operator()
	{
		return parse_precedence_operator(parse_unary(), 0);
	}

	Tree::Node *Parser::parse_precedence_operator(Tree::Node *left, size_t min_precedence)
	{
		while(true)
		{
			Lexeme::Type op = lexeme();
			
			if(!is_precedence_operator(op))
				break;

			size_t precedence = operator_precedences[op - Lexeme::precedence_operators_start];
			
			if(precedence < min_precedence)
				break;
			
			typecheck(left, [&](Tree::Node *left) -> Tree::Node * {
				SourceLoc range = lexer.lexeme;
			
				lexer.step();

				skip_lines();
				
				Tree::Node *right = typecheck(parse_unary());

				while(true)
				{
					Lexeme::Type next_op = lexeme();

					if(!is_precedence_operator(next_op))
						break;

					size_t next_precedence = operator_precedences[next_op - Lexeme::precedence_operators_start];

					if(next_precedence <= precedence)
						break;

					right = typecheck(parse_precedence_operator(right, next_precedence));
				}
			
				auto node = new (fragment) Tree::BinaryOpNode;
			
				node->op = op;
				node->left = left;
				node->right = right;
				node->range = range;

				return node;
			});
		}

		return left;
	}

	Tree::Node *Parser::typecheck(Tree::Node *result)
	{
		if(result->single())
			return result;

		Tree::MultipleExpressionsNode *node = (Tree::MultipleExpressionsNode *)result;

		report(*node->range, "Unexpected multiple expressions");

		return nullptr;
	}
	
	Tree::Node *Parser::parse_range(bool allow_multiple)
	{
		Tree::Node *result = parse_precedence_operator();
		
		if(lexeme() == Lexeme::RANGE_EXCL || lexeme() == Lexeme::RANGE_INCL)
		{
			SourceLoc range = lexer.lexeme;

			typecheck(result, [&](Tree::Node *result) -> Tree::Node * {
				auto node = new (fragment) Tree::RangeNode;
				
				node->range = range;

				node->exclusive = lexeme() == Lexeme::RANGE_EXCL;
			
				lexer.step();
				skip_lines();
			
				node->left = result;
				node->right = parse_operator_expression(allow_multiple);
				
				return node;
			});
		}
		
		return result;
	}
	
	Tree::Node *Parser::parse_splat_expression(bool allow_multiples, bool nested)
	{
		if(matches(Lexeme::MUL))
		{
			if(matches(Lexeme::MUL))
			{
				SourceLoc range = lexer.lexeme;

				while(matches(Lexeme::MUL));

				lexer.lexeme.prev_set(&range);

				report(range, "Nested splat operators are not allowed");
			}

			auto result = new (fragment) Tree::SplatNode;
			
			if(is_expression())
				result->expression = nested ? typecheck(parse_assignment(false, false)) : typecheck(parse_ternary_if(allow_multiples));
			else
				result->expression = 0;

			return result;
		}
		else
			return nested ? parse_assignment(false, false) : parse_ternary_if(allow_multiples);
	}
	
	Tree::Node *Parser::process_rhs(Tree::Node *rhs)
	{
		if(!rhs || rhs->type() != Tree::Node::MultipleExpressions)
			return rhs;

		auto node = static_cast<Tree::MultipleExpressionsNode *>(rhs);
		
		for(auto expr: node->expressions)
		{
			if(expr->expression)
			{
				if(expr->expression->type() == Tree::Node::Splat)
				{
					if(!static_cast<Tree::SplatNode *>(expr->expression)->expression)
						report(expr->range, "Expected argument with splat operator");
				}
			}
			else
				report(expr->range, "Unexpected trailing comma");
		}

		return rhs;
	}

	void Parser::process_lhs(Tree::Node *&lhs, const SourceLoc &range, bool parameter)
	{
		if(!lhs)
			return;

		switch(lhs->type())
		{
			case Tree::Node::Group:
			{
				auto group = static_cast<Tree::GroupNode *>(lhs);
				
				if(group->statements.first && (group->statements.first == group->statements.last))
				{
					auto node = new (fragment) Tree::MultipleExpressionsNode;
					node->expressions.append(new (fragment) Tree::MultipleExpressionNode(group->statements.first, range));

					process_multiple_lhs(node, parameter);
					
					lhs = node;

					return;
				}

				break;
			}

			case Tree::Node::MultipleExpressions:
				process_multiple_lhs(static_cast<Tree::MultipleExpressionsNode *>(lhs), parameter);
				return;
				
			case Tree::Node::CVar:
			case Tree::Node::IVar:
			case Tree::Node::Global:
			case Tree::Node::Constant:
				if(parameter)
					break;
				else
					return;
				
			case Tree::Node::Variable:
				{
					if(parameter)
						error("Variable " + static_cast<Tree::VariableNode *>(lhs)->var->name->get_string() + " already defined");

					return;
				}

			case Tree::Node::Call:
			{
				auto node = static_cast<Tree::CallNode *>(lhs);

				if(node->can_be_var)
				{
					lhs = parse_variable(node->method, node->range);
					
					return;
				}
				else if(!parameter && (node->subscript || (node->arguments.empty() && !node->scope && !node->block_arg)))
				{
					Symbol *mutated = node->method;
					
					if(mutated)
						mutated = Symbol::from_char_array(node->method->string + "=");

					node->method = mutated;

					return;
				}

				break;
			}

			case Tree::Node::Splat:
			{
				auto node = static_cast<Tree::SplatNode *>(lhs);

				process_lhs(node->expression, range);
				
				if(node->expression && (node->expression->type() == Tree::Node::MultipleExpressions))
					report(range, "Cannot use the splat operator on multiple expressions");

				return;
			}

			default:
				break;
		}

		report(range, parameter ? "Not a valid parameter" : "Not an assignable expression");
	}
	
	void Parser::process_multiple_lhs(Tree::MultipleExpressionsNode *node, bool parameter)
	{
		bool seen_splat = false;
		int size = 0;
		int index = 0;
		int offset = 1;
		int splat_index = -1;

		for(auto expr: node->expressions)
		{
			expr->size = size;
			expr->index = index;

			if(!expr->expression || expr->expression->type() == Tree::Node::Splat)
			{
				splat_index = size;
				index = -1;
				offset = -1;

				if(seen_splat)
					report(expr->range, "You can only have one splat operator on the left-hand side");
				else
					seen_splat = true;
			}
			else
			{
				size += 1;
				index += offset;
			}

			process_lhs(expr->expression, expr->range, parameter);
		}
		
		node->expression_count = size;
		node->splat_index = splat_index;
	}

	Tree::Node *Parser::parse_multiple_expressions(bool allow_multiples, bool nested)
	{
		SourceLoc range = lexer.lexeme;
		auto result = parse_splat_expression(allow_multiples, nested);
		
		bool seen_splat = result && (result->type() == Tree::Node::Splat);

		if(allow_multiples)
		{
			Tree::MultipleExpressionsNode *node;

			bool is_multi = lexeme() == Lexeme::COMMA || seen_splat;

			if(is_multi)
			{
				lexer.lexeme.prev_set(&range);

				node = new (fragment) Tree::MultipleExpressionsNode;
				node->expressions.append(new (fragment) Tree::MultipleExpressionNode(result,  range));
			}
			else
				return result;

			while(lexeme() == Lexeme::COMMA)
			{
				SourceLoc comma = lexer.lexeme;

				step_lines();
					
				if(lexeme() == Lexeme::COMMA)
				{
					SourceLoc error;

					while(lexeme() == Lexeme::COMMA)
					{
						step_lines();
					}

					report(error, "Unexpected multiple commas");
				}

				if(is_expression())
				{
					SourceLoc result_range = lexer.lexeme;
					result = parse_splat_expression(allow_multiples, nested);
					lexer.lexeme.prev_set(&result_range);

					if(result)
						node->expressions.append(new (fragment) Tree::MultipleExpressionNode(result, result_range));
				}
				else
					node->expressions.append(new (fragment) Tree::MultipleExpressionNode(0, comma));
			}
			
			lexer.lexeme.prev_set(&range);
			node->range = new (fragment) SourceLoc(range);

			return node;
		}
		else if(seen_splat)
		{
			lexer.lexeme.prev_set(&range);

			auto node = new (fragment) Tree::MultipleExpressionsNode;
			
			node->range = new (fragment) SourceLoc(range);
			node->expressions.append(new (fragment) Tree::MultipleExpressionNode(result,  range));

			return node;
		}
		else
			return result;
	}

	Tree::Node *Parser::build_assignment(Tree::Node *left, bool allow_multiples)
	{
		if(lexeme() == Lexeme::ASSIGN)
		{
			auto result = new (fragment) Tree::AssignmentNode;
			
			lexer.step();
			skip_lines();
			
			result->op = Lexeme::ASSIGN;
			
			result->left = left;
			
			result->right = parse_high_rescue(allow_multiples, true);
			
			return result;
		}
		else
		{
			auto result = new (fragment) Tree::AssignmentNode;
			auto binary_op = new (fragment) Tree::BinaryOpNode;

			result->op = Lexeme::ASSIGN;
			
			result->left = left;
			result->right = binary_op;
			binary_op->left = left;
			binary_op->range = lexer.lexeme;
			binary_op->op = Lexeme::assign_to_operator(lexeme());
			
			lexer.step();
			skip_lines();
			
			binary_op->right = parse_high_rescue(allow_multiples, true);
			
			return result;
		}
	};
	
	Tree::Node *Parser::parse_assignment(bool allow_multiples, bool nested)
	{
		Tree::Node *result = parse_multiple_expressions(allow_multiples, nested);
		
		if(is_assignment_op())
			return process_assignment(result, allow_multiples);
		else
			return result;
	}

	Tree::Node *Parser::process_assignment(Tree::Node *input, bool allow_multiples)
	{
		if(!input)
		{
			lexer.step();
			return parse_high_rescue(allow_multiples, true);
		}

		switch(input->type())
		{
			case Tree::Node::BinaryOp:
			{
				auto node = (Tree::BinaryOpNode *)input;

				node->right = process_assignment(node->right, allow_multiples);

				return node;
			}

			case Tree::Node::MultipleExpressions:
			{
				auto node = (Tree::MultipleExpressionsNode *)input;

				if(lexeme() != Lexeme::ASSIGN)
					error("Can only use regular assign with multiple expression assignment");

				lexer.step();
				skip_lines();

				process_multiple_lhs(node, false);

				auto result = new (fragment) Tree::AssignmentNode;
			
				result->op = Lexeme::ASSIGN;
			
				result->left = node;
			
				result->right = parse_high_rescue(allow_multiples, true);

				return result;
			}
			break;

			case Tree::Node::Call:
			{
				auto node = (Tree::CallNode *)input;
					
				if(!node->subscript)
					if(!node->arguments.empty() || node->scope || node->block_arg)
						break;
				
				if(node->can_be_var)
					return build_assignment(parse_variable(node->method, node->range), allow_multiples);
				else
				{
					Symbol *mutated = node->method;
					
					if(mutated)
						mutated = Symbol::from_char_array(node->method->string + "=");

					if(lexeme() == Lexeme::ASSIGN)
					{
						step_lines();
						
						node->method = mutated;
						
						Tree::Node *argument = parse_high_rescue(allow_multiples, true);

						if(argument)
							node->arguments.append(argument);

						if(node->range)
							lexer.lexeme.prev_set(node->range);

						return input;
					}
					else
					{
						SourceLoc assign_range = lexer.lexeme;
					
						auto group = new (fragment) Tree::GroupNode;
						
						auto object = new (fragment) Tree::VariableNode(scope->alloc_var<Tree::NamedVariable>());

						{
							auto assign = new (fragment) Tree::AssignmentNode;
						
							assign->left = object;
							assign->op = Lexeme::ASSIGN;
							assign->right = node->object;

							group->statements.append(assign);
						}

						node->object = object;

						Tree::VariableNode *block_arg;

						if(node->block_arg)
						{
							block_arg = new (fragment) Tree::VariableNode(scope->alloc_var<Tree::NamedVariable>());

							{
								auto assign = new (fragment) Tree::AssignmentNode;
						
								assign->left = block_arg;
								assign->op = Lexeme::ASSIGN;
								assign->right = node->block_arg->node;

								group->statements.append(assign);
							}

							node->block_arg->node = block_arg;
						}
						
						auto arguments = new (fragment) Tree::VariableNode(scope->alloc_var<Tree::NamedVariable>());

						{
							auto right = new (fragment) Tree::MultipleExpressionsNode;

							for(auto i: node->arguments)
								right->expressions.append(new (fragment) Tree::MultipleExpressionNode(i, SourceLoc()));

							auto assign = new (fragment) Tree::AssignmentNode;
						
							assign->left = arguments;
							assign->op = Lexeme::ASSIGN;
							assign->right = right;

							group->statements.append(assign);

							node->arguments.clear();
							node->variadic = true;
							node->arguments.append(new Tree::SplatNode(arguments));
						}
						
						auto result = new (fragment) Tree::CallNode;
						
						result->object = object;
						result->method = mutated;
						result->block_arg = node->block_arg;
						result->variadic = true;
						result->arguments.append(new Tree::SplatNode(arguments));

						if(node->range)
							result->range = capture();
						else
							result->range = 0;
						
						auto binary_op = new (fragment) Tree::BinaryOpNode;
						
						binary_op->left = node;
						binary_op->range = lexer.lexeme;
						binary_op->op = Lexeme::assign_to_operator(lexeme());
						
						step_lines();
						
						binary_op->right = parse_high_rescue(allow_multiples, true);

						if(binary_op->right->type() == Tree::Node::MultipleExpressions)
							report(assign_range, "Can only use regular assign with multiple expression assignment");

						result->arguments.append(binary_op);

						lexer.lexeme.prev_set(result->range);

						group->statements.append(result);
						
						return group;
					}
				}

				break;
			};
			
			case Tree::Node::Global:
			case Tree::Node::Variable:
			case Tree::Node::Constant: //TODO: Make sure constant's object is not re-evaluated
			case Tree::Node::IVar:
			case Tree::Node::CVar:
				return build_assignment(input, allow_multiples);

			default:
				break;
		}
			
		error("Cannot assign a value to an expression.");
		
		lexer.step();
		skip_lines();

		parse_high_rescue(allow_multiples, true);

		return input;
	}
	
	Tree::Node *Parser::parse_boolean_unary()
	{
		if(lexeme() == Lexeme::KW_NOT)
		{
			lexer.step();
			
			skip_lines();

			auto result = parse_boolean_unary();

			typecheck(result, [&](Tree::Node *result) {
				return new (fragment) Tree::BooleanNotNode(result);
			});

			return result;
		}
		else
		{
			return parse_assignment(true, false);
		}
	}

	Tree::Node *Parser::parse_boolean()
	{
		Tree::Node *result = parse_boolean_unary();
		
		while(lexeme() == Lexeme::KW_AND || lexeme() == Lexeme::KW_OR)
		{
			typecheck(result, [&](Tree::Node *result) -> Tree::Node * {
				auto node = new (fragment) Tree::BinaryOpNode;
				
				node->op = lexeme() == Lexeme::KW_AND ? Lexeme::LOGICAL_AND : Lexeme::LOGICAL_OR;
				node->range = lexer.lexeme;
				node->left = result;
			
				lexer.step();

				skip_lines();
			
				node->right = parse_boolean_unary();

				return node;
			});
		}
		
		return result;
	}
	
	Tree::Node *Parser::parse_alias()
	{
		if(lexeme() == Lexeme::KW_ALIAS)
		{
			SourceLoc range = lexer.lexeme;
			lexer.step();
			
			auto node = new (fragment) Tree::AliasNode;

			lexer.lexeme.allow_keywords = false;

			auto parse_name = [&](Tree::Node *&node) {
				switch(lexeme())
				{
					case Lexeme::SYMBOL:
					case Lexeme::COLON:
						node = parse_symbol();
						break;
						
					default:
					{
						auto symbol = new (fragment) Tree::SymbolNode;

						parse_method_name(symbol->symbol);

						node = symbol;
					}
				}
			};
			
			parse_name(node->new_name);

			lexer.lexeme.allow_keywords = true;

			parse_name(node->old_name);

			lexer.lexeme.prev_set(&range);

			node->range = range;

			return node;
		}
		else
			return parse_boolean();
	}
	
	Tree::Node *Parser::parse_statements()
	{
		skip_lines();
		
		auto group = new (fragment) Tree::GroupNode;
		
		if(!is_sep())
		{
			if(!is_statement())
				return group;

			Tree::Node *node = parse_statement();

			if(node && node->type() == Tree::Node::MultipleExpressions)
			{
				SourceLoc range = lexer.lexeme;

				bool error = is_sep();
				
				skip_seps();
		
				while(is_statement())
				{
					error = true;
					typecheck(parse_statement());
			
					if (!is_sep())
						break;
			
					skip_seps();
				}

				lexer.lexeme.prev_set(&range);

				if(error)
					report(range, "Unexpected statement(s) after multiple expression(s)");

				return node;
			}
			
			if(node)
				group->statements.append(node);
		}
		
		if (!is_sep())
			return group;
			
		skip_seps();
		
		while(is_statement())
		{
			Tree::Node *node = typecheck(parse_statement());
			
			if(node)
				group->statements.append(node);
			
			if (!is_sep())
				break;
			
			skip_seps();
		}

		return group;
	}
	
	bool Parser::is_sep()
	{
		return lexeme() == Lexeme::SEMICOLON || lexeme() == Lexeme::LINE;
	}
	
	void Parser::skip_seps()
	{
		while(is_sep())
			lexer.step();
	}
	
	Tree::Scope *Parser::parse_main()
	{
		fragment = *new Tree::Fragment::Base(Tree::Chunk::main_size);
		
		Tree::Scope *result;
		
		allocate_scope(Tree::Scope::Top, [&] {
			result = scope;
			scope->owner = scope;
			scope->group = parse_group();
		});
		
		match(Lexeme::END);

		lexer.done();
		result->parse_done(*this);
		
		return result;
	}
};

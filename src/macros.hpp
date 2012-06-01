#pragma once

#define MIRB_COMMA() ,

#define MIRB_LIST_0(macro, opt) opt##0(macro)
#define MIRB_LIST_1(macro, opt) opt##1(macro, sep)
#define MIRB_LIST_2(macro, opt) MIRB_LIST_1(macro, opt)opt##SEP() macro(2, 1)opt##AFTER()
#define MIRB_LIST_3(macro, opt) MIRB_LIST_2(macro, opt)opt##SEP() macro(3, 2)opt##AFTER()
#define MIRB_LIST_4(macro, opt) MIRB_LIST_3(macro, opt)opt##SEP() macro(4, 3)opt##AFTER()
#define MIRB_LIST_5(macro, opt) MIRB_LIST_4(macro, opt)opt##SEP() macro(5, 4)opt##AFTER()
#define MIRB_LIST_6(macro, opt) MIRB_LIST_5(macro, opt)opt##SEP() macro(6, 5)opt##AFTER()
#define MIRB_LIST(macro, opt, i) MIRB_LIST_##i(macro, opt ## _)

#define MIRB_ALT_LIST_0(macro, opt) opt##0(macro)
#define MIRB_ALT_LIST_1(macro, opt) opt##1(macro, sep)
#define MIRB_ALT_LIST_2(macro, opt) MIRB_ALT_LIST_1(macro, opt)opt##SEP() macro(2)opt##AFTER()
#define MIRB_ALT_LIST_3(macro, opt) MIRB_ALT_LIST_2(macro, opt)opt##SEP() macro(3)opt##AFTER()
#define MIRB_ALT_LIST_4(macro, opt) MIRB_ALT_LIST_3(macro, opt)opt##SEP() macro(4)opt##AFTER()
#define MIRB_ALT_LIST_5(macro, opt) MIRB_ALT_LIST_4(macro, opt)opt##SEP() macro(5)opt##AFTER()
#define MIRB_ALT_LIST_6(macro, opt) MIRB_ALT_LIST_5(macro, opt)opt##SEP() macro(6)opt##AFTER()
#define MIRB_ALT_LIST(macro, opt, i) MIRB_ALT_LIST_##i(macro, opt ## _)

#define MIRB_LOCAL_STATEMENT_LIST_OPT_0(macro)
#define MIRB_LOCAL_STATEMENT_LIST_OPT_1(macro, sep) macro(1, 0);
#define MIRB_LOCAL_STATEMENT_LIST_OPT_SEP()
#define MIRB_LOCAL_STATEMENT_LIST_OPT_AFTER() ;
#define MIRB_LOCAL_STATEMENT_LIST(macro, i) MIRB_LIST(macro, MIRB_LOCAL_STATEMENT_LIST_OPT, i)

#define MIRB_STATEMENT_LIST_OPT_0(macro) macro(0)
#define MIRB_STATEMENT_LIST_OPT_1(macro, sep) macro(0); macro(1)
#define MIRB_STATEMENT_LIST_OPT_SEP() ;
#define MIRB_STATEMENT_LIST_OPT_AFTER()
#define MIRB_STATEMENT_LIST(macro, i) MIRB_ALT_LIST(macro, MIRB_STATEMENT_LIST_OPT, i)

#define MIRB_STATEMENT_LIST_NO_ZERO_OPT_0(macro) 
#define MIRB_STATEMENT_LIST_NO_ZERO_OPT_1(macro, sep) macro(1)
#define MIRB_STATEMENT_LIST_NO_ZERO_OPT_SEP() ;
#define MIRB_STATEMENT_LIST_NO_ZERO_OPT_AFTER()
#define MIRB_STATEMENT_LIST_NO_ZERO(macro, i) MIRB_ALT_LIST(macro, MIRB_STATEMENT_LIST_NO_ZERO_OPT, i)

#define MIRB_STATEMENT_MAX 6

#define MIRB_COMMA_LIST_OPT_0(macro)
#define MIRB_COMMA_LIST_OPT_1(macro, sep) macro(1, 0)
#define MIRB_COMMA_LIST_OPT_SEP() ,
#define MIRB_COMMA_LIST_OPT_AFTER()
#define MIRB_COMMA_LIST(macro, i) MIRB_LIST(macro, MIRB_COMMA_LIST_OPT, i)

#define MIRB_COMMA_AFTER_LIST_OPT_0(macro)
#define MIRB_COMMA_AFTER_LIST_OPT_1(macro, sep) macro(1, 0),
#define MIRB_COMMA_AFTER_LIST_OPT_SEP()
#define MIRB_COMMA_AFTER_LIST_OPT_AFTER() ,
#define MIRB_COMMA_AFTER_LIST(macro, i) MIRB_LIST(macro, MIRB_COMMA_AFTER_LIST_OPT, i)

#define MIRB_COMMA_BEFORE_LIST_OPT_0(macro)
#define MIRB_COMMA_BEFORE_LIST_OPT_1(macro, sep) , macro(1, 0)
#define MIRB_COMMA_BEFORE_LIST_OPT_SEP() ,
#define MIRB_COMMA_BEFORE_LIST_OPT_AFTER()
#define MIRB_COMMA_BEFORE_LIST(macro, i) MIRB_LIST(macro, MIRB_COMMA_BEFORE_LIST_OPT, i)

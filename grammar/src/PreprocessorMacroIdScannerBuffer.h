/*****************************************************************************
 * This file was generated automatically by Parser.
 * Do not edit this file manually.
 * 
 * This file was generated using Parser-1.0.0 compiled in Jun  9 2009 00:43:14
 * 
 * File created in 2009-06-29 22:24:47
 *****************************************************************************/

#ifndef PREPROCESSORMACROIDSCANNERBUFFER_H
#define PREPROCESSORMACROIDSCANNERBUFFER_H

enum PreprocessorMacroIdScannerBuffer_tokens {
	PREPROCESSORMACROIDSCANNERBUFFER_TOKEN_CONSTANT = 2,
	PREPROCESSORMACROIDSCANNERBUFFER_TOKEN_IDENTIFIER = 0,
	PREPROCESSORMACROIDSCANNERBUFFER_TOKEN_STRING_LITERAL = 1,
	PREPROCESSORMACROIDSCANNERBUFFER_TOKEN_TEXT = 4,
	PREPROCESSORMACROIDSCANNERBUFFER_TOKEN_WHITESPACE = 3
};

extern unsigned char prepro_macro_id_buffer_scanner[];
extern unsigned int prepro_macro_id_buffer_scanner_size;

#endif
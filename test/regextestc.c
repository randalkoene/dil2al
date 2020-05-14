/* Little test program */

#include <stdio.h>
#include <sys/types.h>
#include "regex-gnu.h"

main () {
	regex_t re;
	int res;
	res = regcomp(&re,"Hello World",REG_EXTENDED|REG_NOSUB);
	res = regexec(&re,"This is a Hello World program.",(size_t) 0, NULL, 0);
	regfree(&re);
	printf("%d\n",res);
	regcomp(&re,"[ \n\t\r\v\f]+",REG_EXTENDED);
/*	BigRegex brx("Hello World");
//const BigRegex lBRXwhite("1[ \n\t\r\v\f]+", 1);
//const BigRegex lBRXint("-?[0-9]+", 1);
//const BigRegex lBRXdouble("-?\\(\\([0-9]+\\.[0-9]*\\)\\|\\([0-9]+\\)\\|\\(\\.[0-9]+\\)\\)\\([eE][---+]?[0-9]+\\)?", 1, 200);
//const BigRegex lBRXalpha("[A-Za-z]+", 1);
//const BigRegex lBRXlowercase("[a-z]+", 1);
//const BigRegex lBRXuppercase("[A-Z]+", 1);
//const BigRegex lBRXalphanum("[0-9A-Za-z]+", 1);
//const BigRegex lBRXidentifier("[A-Za-z_][A-Za-z0-9_]*", 1);
*/	exit(0);

}

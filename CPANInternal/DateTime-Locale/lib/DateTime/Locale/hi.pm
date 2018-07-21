###########################################################################
#
# This file is auto-generated by the Perl DateTime Suite time locale
# generator (0.02).  This code generator comes with the
# DateTime::Locale distribution in the tools/ directory, and is called
# generate_from_icu.
#
# This file as generated from the ICU XML locale data.  See the
# LICENSE.icu file included in this distribution for license details.
#
# This file was generated from the source file hi.xml.
# The source file version number was 1.2, generated on
# 2004-08-27.
#
# Do not edit this file directly.
#
###########################################################################

package DateTime::Locale::hi;

use strict;

BEGIN
{
    if ( $] >= 5.006 )
    {
        require utf8; utf8->import;
    }
}

use DateTime::Locale::root;

@DateTime::Locale::hi::ISA = qw(DateTime::Locale::root);

my @day_names = (
"सोमवार",
"मंगलवार",
"बुधवार",
"गुरुवार",
"शुक्रवार",
"शनिवार",
"रविवार",
);

my @day_abbreviations = (
"सोम",
"मंगल",
"बुध",
"गुरु",
"शुक्र",
"शनि",
"रवि",
);

my @month_names = (
"जनवरी",
"फरवरी",
"मार्च",
"अप्रैल",
"मई",
"जून",
"जुलाई",
"अगस्त",
"सितम्बर",
"अक्तूबर",
"नवम्बर",
"दिसम्बर",
);

my @month_abbreviations = (
"जनवरी",
"फरवरी",
"मार्च",
"अप्रैल",
"मई",
"जून",
"जुलाई",
"अगस्त",
"सितम्बर",
"अक्तूबर",
"नवम्बर",
"दिसम्बर",
);

my @am_pms = (
"पूर्वाह्न",
"अपराह्न",
);

my @eras = (
"ईसापूर्व",
"सन",
);



sub day_names                      { \@day_names }
sub day_abbreviations              { \@day_abbreviations }
sub month_names                    { \@month_names }
sub month_abbreviations            { \@month_abbreviations }
sub am_pms                         { \@am_pms }
sub eras                           { \@eras }



1;


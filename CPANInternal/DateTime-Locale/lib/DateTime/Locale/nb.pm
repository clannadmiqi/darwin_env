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
# This file was generated from the source file nb.xml.
# The source file version number was 1.3, generated on
# 2005-01-28T21:56:22Z.
#
# Do not edit this file directly.
#
###########################################################################

package DateTime::Locale::nb;

use strict;

BEGIN
{
    if ( $] >= 5.006 )
    {
        require utf8; utf8->import;
    }
}

use DateTime::Locale::root;

@DateTime::Locale::nb::ISA = qw(DateTime::Locale::root);

my @day_names = (
"mandag",
"tirsdag",
"onsdag",
"torsdag",
"fredag",
"lørdag",
"søndag",
);

my @day_abbreviations = (
"man\.",
"tir\.",
"ons\.",
"tor\.",
"fre\.",
"lør\.",
"søn\.",
);

my @day_narrows = (
"M",
"T",
"O",
"T",
"F",
"L",
"S",
);

my @month_names = (
"januar",
"februar",
"mars",
"april",
"mai",
"juni",
"juli",
"august",
"september",
"oktober",
"november",
"desember",
);

my @month_abbreviations = (
"jan",
"feb",
"mar",
"apr",
"mai",
"jun",
"jul",
"aug",
"sep",
"okt",
"nov",
"des",
);

my @month_narrows = (
"J",
"F",
"M",
"A",
"M",
"J",
"J",
"A",
"S",
"O",
"N",
"D",
);

my @eras = (
"f\.Kr\.",
"e\.Kr\.",
);

my $date_parts_order = "dmy";


sub day_names                      { \@day_names }
sub day_abbreviations              { \@day_abbreviations }
sub day_narrows                    { \@day_narrows }
sub month_names                    { \@month_names }
sub month_abbreviations            { \@month_abbreviations }
sub month_narrows                  { \@month_narrows }
sub eras                           { \@eras }
sub full_date_format               { "\%A\ \%\{day\}\.\ \%B\ \%\{ce_year\}" }
sub long_date_format               { "\%\{day\}\.\ \%B\ \%\{ce_year\}" }
sub medium_date_format             { "\%\{day\}\.\ \%b\.\ \%\{ce_year\}" }
sub short_date_format              { "\%d\.\%m\.\%y" }
sub full_time_format               { "kl\.\ \%H\.\%M\.\%S\ \%\{time_zone_long_name\}" }
sub long_time_format               { "\%H\.\%M\.\%S\ \%\{time_zone_long_name\}" }
sub medium_time_format             { "\%H\.\%M\.\%S" }
sub short_time_format              { "\%H\.\%M" }
sub date_parts_order               { $date_parts_order }



1;


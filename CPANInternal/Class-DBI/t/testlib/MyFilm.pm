package MyFilm;

BEGIN { unshift @INC, './t/testlib'; }
use base 'MyBase';
use MyStarLink;

use strict;

__PACKAGE__->set_table();
__PACKAGE__->columns(All => qw/filmid title/);
__PACKAGE__->hasa_list(MyStarLink => ['film'] => '_stars');

sub _carp { }

sub stars { map $_->star, shift->_stars }

sub create_sql {
	return qq{
    filmid  TINYINT NOT NULL AUTO_INCREMENT PRIMARY KEY,
    title   VARCHAR(255)
  };
}

1;


#!/usr/bin/perl
use XML::Simple;
use warnings;
use strict;
use Data::Dumper;

my $xml = XML::Simple->new();
my $users = $xml->XMLin("data/user.xml", KeyAttr => {});
my %l;

# Snarf through the XML data. Build a hash: 
#  username => lastuse
# Only for users with no items and no money. 
# For some reason last_use is floating point so let's fix that.
foreach my $user (@{$users->{user}}) {
  if ($user->{'used_stalls'} == 0 && $user->{'money'} == 0) {
    $user->{'last_use'} = 0 unless ($user->{'last_use'});
    $l{ $user->{'name'} } = int($user->{'last_use'});
 }
}


print("Last used\t\t\tUsername\n");
foreach (  sort { $l{$a} cmp $l{$b} || $a cmp $b } keys %l  ) {
  print gmtime($l{$_}) . "\t$_\n";
}



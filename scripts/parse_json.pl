#!/usr/bin/perl
use strict;
use warnings;
use JSON qw( decode_json );
use Cwd;
use File::Copy;

# Take in 1 arg which is the json file path
# Return two arrays of arrays  
sub parse_json {
  my $json_fn = $_[0];
  open my $fh, '<', $json_fn or die "error opening $json_fn: $!";
  my $json = do { local $/; <$fh> };

  my $decoded = decode_json($json);

  my @RoCC_accels = ();
  my $i;
  print("\nRoCC Accels: \n");
  for( $i = 0; $i < 4; $i = $i + 1 ){
   
    if ((exists $decoded -> {'RoCC'}{"custom$i"}{'pgm'}) and (exists $decoded -> {'RoCC'}{"custom$i"}{'func'} )){
      my $pgm = $decoded -> {'RoCC'}{"custom$i"}{'pgm'}; 
      my $func = $decoded -> {'RoCC'}{"custom$i"}{'func'};
      if(($pgm ne "") and ($func ne "")){
        print("\tpgm: $pgm\t func: $func\n");
        my @tup = ();
        push (@tup, $pgm);
        push (@tup, $func);
        push (@RoCC_accels, \@tup); 
      }
    }
  }

  print("TLL2 Accels: \n");
  my @TLL2_accels = ();
  if (exists $decoded -> {'TLL2'}){
    my @TLL2_arr = @{$decoded-> {'TLL2'}}; 
    foreach my $accel (@TLL2_arr) { 
      if( (exists $accel->{'pgm'}) and (exists $accel->{'func'} and (exists $accel->{'addr'}))){
        my $pgm = $accel->{'pgm'};  
        my $func = $accel->{'func'}; 
        my $addr = $accel->{'addr'};
        if ($pgm ne "" and $func ne "" and $addr ne ""){

          print("\tpgm: $pgm\t func: $func\t addr: $addr\n");
          my @tup = ();
          push (@tup, $pgm);
          push (@tup, $func);
          push (@tup, $addr);
          push (@TLL2_accels, \@tup); 
        }
      }
    }
  }
  return (\@RoCC_accels, \@TLL2_accels);
}

1;

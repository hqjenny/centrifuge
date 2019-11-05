#!/usr/bin/perl
use warnings;
use strict;
use Cwd;
use File::Copy;

my $rdir = $ENV{'RDIR'};
print $rdir;
if ((not defined($rdir)) or $rdir eq '') {
    print("Please source centrifuge/env.sh!\n");
    exit();
}

sub compile_sw_bm{

    my @accel_tuples= @{$_[0]};
    my %tasks = %{$_[1]};
  
    foreach my $accel_tuple_ref (@accel_tuples) {
      print($accel_tuple_ref);
      my @accel_tuple = @{$accel_tuple_ref};

      my $pgm = $accel_tuple[0];
      my $func = $accel_tuple[1];
      my $bm_path = $accel_tuple[2];
      my $bm_path_c = $bm_path.'/src/main/c/';

      my $is_rocc = $accel_tuple[3];
      my $idx_addr = $accel_tuple[4];

      my $prefix=" ";

      my $num_args = scalar @accel_tuple;
      if ($num_args > 5) {
        $prefix = $accel_tuple[5];
      }
     
      $ENV{'PGM'} = $pgm;
      $ENV{'FUNC'} = $func;
      my $PGM = $pgm;
      my $FUNC = $func;
      my $RDIR = $rdir;

      chdir("$bm_path/src/main/c/") or die $!;
      
      my $dir = getcwd;
      print("$dir\n");

      unless (-e 'compile-bm.sh') {
          system("echo 'make clean' > compile-bm.sh");
          system("echo 'make' >> compile-bm.sh");
          system("chmod +x compile-bm.sh");
      }

      # Run compile-bm.sh for compiling bare-metal software
      system("./compile-bm.sh");
    }
}

1;

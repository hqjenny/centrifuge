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

sub generate_accel{

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
     
      print("Pgm: ".$pgm."\n");
      print("Func: ".$func."\n");
      print("Path: ".$bm_path."\n");
      print("Is RoCC not TL?: ".$is_rocc."\n");
      print("RoCC Idx or TL Addr: ".$idx_addr."\n");
      print("Prefix: ".$prefix."\n");
      $ENV{'PGM'} = $pgm;
      $ENV{'FUNC'} = $func;
      my $PGM = $pgm;
      my $FUNC = $func;
      my $RDIR = $rdir;

      system("mkdir -p $bm_path/src/main/c");
      chdir("$bm_path/src/main/c/") or die $!;
      system("cp -H $RDIR/tools/centrifuge/examples/${PGM}/* $bm_path_c");
      system("cp -H $RDIR/tools/centrifuge/scripts/run_hls.pl $bm_path_c");

      # Specialize the Makefile for this function
      system("sed -i 's/^FUNC=.*/FUNC=$func/g' $bm_path_c/Makefile");
      
      my $dir = getcwd;
      print("$dir\n");
      #next;

      if ($tasks{'accel_hls'}){
        system("perl run_hls.pl ${PGM} ${FUNC} $prefix"); 
      }

      if ($tasks{'accel_sw'}){
          # Add compile-bm.sh for compile_sw and fireMarshal
          system("echo 'make clean' > compile-bm.sh");
          system("echo 'make' >> compile-bm.sh");
          system("chmod +x compile-bm.sh");
    	
          # Generate fireMarshal config 
          my $marshal_config_sw_bm_json = "$rdir/tools/centrifuge/scripts/templates/marshal_config_sw_bm_json"; 
          open my $fh, '<', $marshal_config_sw_bm_json or die "error opening $marshal_config_sw_bm_json $!";
          my $marshal_config_sw_bm_template = do { local $/; <$fh> };
    
          my $name = $pgm.'-bare-'.'sw-bm';  
          my $bin = "$pgm.bm.rv";  
          my $marshal_config_sw_bm = $marshal_config_sw_bm_template; 
          $marshal_config_sw_bm =~ s/WORKDIR/$bm_path_c/;
          $marshal_config_sw_bm =~ s/HOSTINIT/compile-bm.sh/;
    
          # For orig sw run
          my $marshal_config_sw_bm_accel = $marshal_config_sw_bm; 
          $marshal_config_sw_bm =~ s/NAME/$name/;
          $marshal_config_sw_bm =~ s/BIN/$bin/;
          open FILE, ">$name.json";
          print FILE $marshal_config_sw_bm;
          close FILE;
    
          # For accel run 
          $name = $name.'_accel'; 
          $bin = "$pgm.bm_accel.rv";
          $marshal_config_sw_bm_accel =~ s/NAME/$name/;
          $marshal_config_sw_bm_accel =~ s/BIN/$bin/;
          open FILE, ">$name.json";
          print FILE $marshal_config_sw_bm_accel;
          close FILE;
      }
      if ($is_rocc) {
          system("cp -H $RDIR/tools/centrifuge/scripts/run_chisel.pl $bm_path_c");
          system("cp -H $RDIR/tools/centrifuge/scripts/generate_wrapper.pl $bm_path_c");
          if ($tasks{'accel_chisel'}){
            system("perl run_chisel.pl ${PGM} ${FUNC} $prefix");
          }
          if ($tasks{'accel_sw'}){
            system("$RDIR/tools/centrifuge/scripts/generate_wrapper.py --fname ${FUNC} " .
              "--prefix $prefix " .
              "--base $idx_addr " .
              "--mode 'rocc' "      .
              "--source $bm_path");
          }
          #system("make clean");
          #system("make CUSTOM_INST=1");
      } else {
          system("cp -H $RDIR/tools/centrifuge/scripts/run_chisel_tl.pl $bm_path_c");
          system("cp -H $RDIR/tools/centrifuge/scripts/generate_wrapper_tl.pl $bm_path_c");
          if ($tasks{'accel_chisel'}){
            system("perl run_chisel_tl.pl ${PGM} ${FUNC} $idx_addr $prefix");
          }

          if ($tasks{'accel_sw'}){
            system("$RDIR/tools/centrifuge/scripts/generate_wrapper.py --fname ${FUNC} " .
              "--base $idx_addr " .
              "--prefix $prefix " .
              "--mode 'tl' "      .
              "--source $bm_path");
          }
          #system("make clean");
          #system("make CUSTOM_DRIVER=1");
      }
   }
}

# Example with RoCC and TL accel
#my @input = (["vadd", "vadd", "$rdir/sim/target-rtl/firechip/hls_vadd_vadd/src/main/c", 1, "0", "rocc0_"], ["vadd_tl", "vadd", "$rdir/sim/target-rtl/firechip/hls_vadd_tl_vadd/src/main/c", 0, "0x20000", "tl0_"]);
#generate_accel(\@input);
1;

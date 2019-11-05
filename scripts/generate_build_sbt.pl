#!/usr/bin/perl
use warnings;
use strict;
use Cwd;
use File::Copy;

sub generate_build_sbt {

    my $rdir = $ENV{'RDIR'};
    print $rdir;
    if ((not defined($rdir)) or $rdir eq '') {
        print("Please source centrifuge/env.sh!\n");
        exit();
    }

    # back up the build sbt fie
    copy("$rdir/build.sbt","$rdir/build.sbt.bk") or die "Copy failed: $!";

    open SBT, ">$rdir/build.sbt";

    # hash of all hls bm and its path
    my $soc_name = $_[0];
    my %bm_path = %{$_[1]};

    my $build_sbt_template = "$rdir/tools/centrifuge/scripts/templates/build_sbt_template"; 
    open my $fh, '<', $build_sbt_template or die "error opening $build_sbt_template $!";
    my $build_sbt = do { local $/; <$fh> };

    # print list of hls accels 
    my $dep_template='lazy val BM = (project in file("PATH"))
  .dependsOn(rocketchip, testchipip, midasTargetUtils, icenet)
  .settings(commonSettings)
    ';

    keys %bm_path;
    
    while(my($bm, $path) = each %bm_path) {
        my $dep = $dep_template;
        $dep =~ s/BM/$bm/; 
        $dep =~ s/PATH/$path/; 
        $build_sbt = $build_sbt."\n".$dep;
    }
    
    my @bm = (keys %bm_path);
    my $bm_size = @bm; 
    my $bms = '';
    if ($bm_size > 0) {
       $bms = ", ".join(", ", @bm);
    }

    my $soc_template = '
lazy val SOC_NAME = conditionalDependsOn(project in file("generators/SOC_NAME"))
  .dependsOn(boom, hwacha, sifive_blocks, sifive_cache, utilitiesBMS)
  .settings(commonSettings)
';
    my $soc = $soc_template;
    $soc =~ s/SOC_NAME/$soc_name/g;
    $soc =~ s/BMS/$bms/;
    $build_sbt = $build_sbt.$soc;
    my $firechip_template = '
lazy val example = conditionalDependsOn(project in file("generators/example"))
  .dependsOn(boom, hwacha, sifive_blocks, sifive_cache, utilities, sha3, SOC_NAME)
  .settings(commonSettings)

lazy val firechip = (project in file("generators/firechip"))
  .dependsOn(SOC_NAME, example, icenet, testchipip, tracegen, midasTargetUtils, midas, firesimLib % "test->test;compile->compile")
  .settings(
    commonSettings,
    testGrouping in Test := isolateAllTests( (definedTests in Test).value )
  )
';
    my $firechip_dep = $firechip_template;
    $firechip_dep =~ s/SOC_NAME/$soc_name/g;
    $build_sbt = $build_sbt.$firechip_dep;
    
    print SBT $build_sbt;
    close SBT;
}

1;

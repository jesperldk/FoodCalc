#
# This program reads a USDA Nutirent Database in ASCII fomat and writes it in a format
# usable by the FoodCalc program.
# The USDA ASCII files are available at http://www.nal.usda.gov/fnic/foodcomp.
#
# This is a Perl program and requires that you have installed Perl at you computer.
# Execute the program with:
#    perl FCusda.pl
# The program expects to find the following USDA files in the current directory:
# 	FOOD_DES.TXT
#   NUT_DATA.TXT
#		NUTR_DEF.TXT
#
# It will write a FoodCalc foods file with the name FCUSDA.TXT. The file will use
# the INFOODS tagnames as FoodCalc field names.
#
# You can get the FoodCalc program at http://www.ibt.ku.dk/jesper/FoodCalc.
#
# Programed by Jesper Lauritsen, 19980617
# 19980821 Jesper Lauritsen: added fields Desc, Short_Desc and Non_edible
#

$fd = "FOOD_DES.TXT";
$nu = "NUT_DATA.TXT";
$nd = "NUTR_DEF.TXT";
$fc = "FCUSDA.TXT";

open(ND,"<$nd") || die "Could not open $nd - $!";
open(NU,"<$nu") || die "Could not open $nu - $!";
open(FD,"<$fd") || die "Could not open $fd - $!";
open(FC,">$fc") || die "Could not open $fc - $!";

print FC "NDB_No,Desc,Shrt_Desc,FdGp_Cd,Refuse,Non_edible,N_Factor,Pro_Factor,Fat_Factor,CHO_Factor";
$nn = 7;

$n = 0;
while (<ND>) {
	chop;
	if (/^~(\d+)~\^~\w+~\^~(\w+)~/) {
		print FC ",$2";
		$nd[$n++] = 0+$1;
		$nd{0+$1} = $2;
		$nn++;
	} else {
		print "Error in $nd line: $_\n";
	}
}

print FC "\n";

while (<FD>) {
	chop;
	if (/^~(\d+)~\^(.*)/) {
		$fd{0+$1} = $2;
	} else {
		print "Error in $fd line: $_\n";
	}
}

$fn = 0;
$id = 0;
while (<NU>) {
	chop;
	if (!/^~(\d+)~\^~(\d+)~\^([\.\d]+)\^[\.\d]+\^([\.\d]+)\^~\d*~$/) {
		print "Error in $nu line: $_\n";
	} else {
		if ($1 != $id) {
			if ($id != 0) {
				nu();
			}
			$id = 0+$1;
			%nu = ();
			print "\r".(++$fn);
		}
		if (!defined($nd{0+$2})) {print "Nutrient id $2 not found in $nd.\n"}
		$nu{0+$2} = $3;
	}
}
nu();

print "\rWrote $fc with $nn fields and $fn foods.\n";

sub nu {
	if (!defined($fd{$id})) {
		print "Food $id not found in $fd.\n";
	} elsif ($fd{$id} !~ /^~(\d+)~\^~(.+)~\^~(.+)~\^~.*~\^([\.\d]+)\^~.*~\^([\.\d]+)\^([\.\d]+)\^([\.\d]+)\^([\.\d]+)$/) {
		print "Error in $fd line: ~$id~\^$fd{$id}\n";
	} else {
		my($desc) = $2; $desc =~ s/\"/\"\"/g;
		my($sdesc) = $3; $sdesc =~ s/\"/\"\"/g;
		print FC "$id,\"$desc\",\"$sdesc\",$1,$4,".($4/100).",$5,$6,$7,$8";
		foreach (@nd) {print FC ",",$nu{$_}}
		print FC "\n";
	}
}

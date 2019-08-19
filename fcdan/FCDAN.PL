#
# Dette program læser levnedsmiddeltabellen (udgaven fra 1996) som modtaget fra
# Veterinær- og Fødevaredirektoratet. Det vil sige du skal have levnedsmiddel-
# tabellen *før* du kan køre dette program. Til forskningsformål kan tabellen
# købes fra Veterinær- og Fødevaredirektoratet for nogle få tusinde kroner.
#
# Dette program er et Perl-program, og du skal have Perl installeret for at
# køre det!
# Perl kan downloades fra www.perl.com (eller direkte fra www.activestate.com,
# hvis man kører Windows 95/98/NT).
#
# Levnedsmiddeltabellen skal findes i filen maintabl.96 i det katalog som
# fcdan.pl køres fra. Kør på følgende måde:
#    perl fcdan.pl
#
# Resultatet er følgende filer:
# foodtbl.txt   Levnedsmiddeltabellen i en form, der kan bruges til FoodCalc
#               programmet.
# indhstof.txt  Liste over feltnavnene i foodtbl.txt, samt sammenhængen mellem
#               disse og Veterinær- og Fødevaredirektoratets stofnumre.
# foodid.txt    Liste over fødevarenumre (lig Veterinær- og Fødevaredirektoratets
#               varenumre) og fødevarenavne. Sorteret efter fødevarenumre.
# fooddan.txt   Som foodid.txt, men sorteret efter fødevarenavn.
# foodgrp.txt   Som foodid.txt, men sorteret efter fødevaregruppe og fødevarenavn.
# tabtbl.txt    Liste over tab ved tilberedning på en form, som kan brugs til
#               FoodCalc programmet. Disse data er indtastet fra Levnedsmiddel-
#               tabellen (udgaven fra 1996), Tabel 10.
# hgrp.txt      Hovedgrupper.
# ugrp.txt      Undergrupper.
#
# Programmet laver korte feltnavne, hvis det i stedet køres på følgende måde:
#    perl fcdan.pl kort
#
# Programmeret af Jesper Lauritsen, 22. januar 1998 for Kræftens Bekæmpelse.
# Enkelte rettelser af Jesper Lauritsen, 2. juni 1998
# Bruger nu FoodCalc text fields, Jesper Lauritsen, 13. juli 1998
# Version 1.3, Jesper Lauritsen, 5. januar 1998:
#   Også en udgave med korte felt navne.
#   Alle feltnavne ændret!
#   også hgrp.txt og ugrp.txt
#

$in = "maintabl.96";
$out = "foodtbl.txt";
$kort = (($ARGV[0] eq "kort")?1:0);

open(IN,"<$in") || ((print STDERR "could not open $in - $!\n"), exit(1));
open(OUT,">$out") || ((print STDERR "could not open $out - $!\n"), exit(1));

%fields = (
	1,		"Prot",
	3,		"Fedt",
	8,		"Kulhy",
	9,		"Suktil",
	10,		"Kfibre",
	11,		"Alko",
	12,		"Aske",
	13,		"Vand",
	14,		"Avit",
	15,		"Retino",
	16,		"Betaca",
	23,		"Dvit",
	26,		"Evit",
	27,		"Alfato",
	35,		"K1vit",
	36,		"Thiami",
	37,		"Ribofl",
	38,		"NENiac",
	39,		"Niacin",
	40,		"Trypto",
	41,		"B6vit",
	45,		"Pantot",
	46,		"Biotin",
	47,		"Folaci",
	49,		"B12vit",
	50,		"Cvit",
	56,		"Natriu",
	57,		"Kalium",
	58,		"Calciu",
	59,		"Magnes",
	60,		"Fosfor",
	61,		"jern",
	64,		"Kobber",
	65,		"Zink",
	66,		"Jod",
	67,		"Mangan",
	68,		"Chrom",
	69,		"Selen",
	72,		"Nikkel",
	124,	"Frukto",
	126,	"Glukos",
	130,	"Laktos",
	131,	"Maltos",
	132,	"Saccha",
	138,	"Sukker",
	144,	"Stivel",
	174,	"C4x0",
	175,	"C6x0",
	176,	"C8x0",
	177,	"C10x0",
	178,	"C12x0",
	179,	"C14x0",
	180,	"C15x0",
	181,	"C16x0",
	182,	"C17x0",
	183,	"C18x0",
	184,	"C20x0",
	185,	"C22x0",
	186,	"C24x0",
	187,	"Fedtam",
	188,	"Fedtms",
	189,	"C14x1",
	190,	"C15x1",
	191,	"C16x1",
	192,	"C17x1",
	193,	"C18x1",
	194,	"C18x1n7",
	197,	"C20x1",
	198,	"C22x1",
	200,	"C22x1n11",
	201,	"Fedtau",
	202,	"Fedtus",
	203,	"C18x2",
	206,	"C18x3n3",
	208,	"C18x4n3",
	210,	"C20x3",
	211,	"C20x4n6",
	212,	"C20x5n3",
	214,	"C22x5n3",
	215,	"C22x6n3",
	216,	"Fedtap",
	217,	"Fedtps",
	223,	"Choles",
	236,	"Isoleu",
	237,	"Leucin",
	238,	"Lysin",
	239,	"Methio",
	240,	"Cystin",
	241,	"Phenyl",
	242,	"Thyros",
	243,	"Threon",
	244,	"Trypto",
	245,	"Valin",
	246,	"Argini",
	247,	"Histid",
	248,	"Alanin",
	249,	"Aspara",
	250,	"Glutam",
	251,	"Glycin",
	252,	"Prolin",
	253,	"Serin",
);
%fieldsl = (
	1,		"Protein_tot",
	3,		"Fedt_tot",
#	4,		"Fedtsyrer_mættede", 		# = 188
#	5,		"Fedtsyrer_monoumætt", 	# = 202
#	6,		"Fedtsyrer_polyumætt",	# = 217
	8,		"Kulhydrat_tot",
	9,		"Sukker_tilsat",
	10,		"Kostfibre",
	11,		"Alkohol",
	12,		"Aske",
	13,		"Vand",
	14,		"A_vitamin",
	15,		"Retinol",
	16,		"Beta_caroten",
	23,		"D_vitamin",
	26,		"E_vitamin",
	27,		"Alfa_tokoferol",
	35,		"K1_vitamin",
	36,		"Thiamin",
	37,		"Riboflavin",
	38,		"NE_Niacin",
	39,		"Niacin",
	40,		"Tryptofans_bidrag",
	41,		"B6_vitamin",
	45,		"Pantotensyre",
	46,		"Biotin",
	47,		"Folacin",
	49,		"B12_vitamin",
	50,		"C_vitamin",
	56,		"Natrium",
	57,		"Kalium",
	58,		"Calcium",
	59,		"Magnesium",
	60,		"Fosfor",
	61,		"jern",
	64,		"Kobber",
	65,		"Zink",
	66,		"Jod",
	67,		"Mangan",
	68,		"Chrom",
	69,		"Selen",
	72,		"Nikkel",
	124,	"Fruktose",
	126,	"Glukose",
	130,	"Laktose",
	131,	"Maltose",
	132,	"Saccharose",
	138,	"Sukkerater_ialt",
	144,	"Stivelse",
	174,	"C4:0",
	175,	"C6:0",
	176,	"C8:0",
	177,	"C10:0",
	178,	"C12:0",
	179,	"C14:0",
	180,	"C15:0",
	181,	"C16:0",
	182,	"C17:0",
	183,	"C18:0",
	184,	"C20:0",
	185,	"C22:0",
	186,	"C24:0",
	187,	"Fedtsyrer_andre_mætt",
	188,	"Fedtsyrer_mætt_sum",
	189,	"C14:1",
	190,	"C15:1",
	191,	"C16:1",
	192,	"C17:1",
	193,	"C18:1",
	194,	"C18:1pn_7",
	197,	"C20:1",
	198,	"C22:1",
	200,	"C22:1pn11",
	201,	"Fedtsyrer_andre_monoumætt",
	202,	"Fedtsyrer_monoumætt_sum",
	203,	"C18:2",
	206,	"C18:3pn_3",
	208,	"C18:4pn_3",
	210,	"C20:3",
	211,	"C20:4pn_6",
	212,	"C20:5pn_3",
	214,	"C22:5pn_3",
	215,	"C22:6pn_3",
	216,	"Fedtsyrer_andre_polyumætt",
	217,	"Fedtsyrer_polyumætt_sum",
	223,	"Cholesterol",
	236,	"Isoleucin",
	237,	"Leucin",
	238,	"Lysin",
	239,	"Methionon",
	240,	"Cystin",
	241,	"Phenylalanin",
	242,	"Thyrosin",
	243,	"Threonin",
	244,	"Tryptofan",
	245,	"Valin",
	246,	"Arginin",
	247,	"Histidin",
	248,	"alanin",
	249,	"Asparaginsyre",
	250,	"Glutaminsyre",
	251,	"Glycin",
	252,	"Prolin",
	253,	"Serin",
);
if (!$kort) {%fields = %fieldsl}
@fieldnos = sort {$a <=> $b;} keys(%fields);
#foreach (@fieldnos) {print "$_ $fields{$_},  "} exit;
if ($kort) {
	print OUT "Foodid,Foodnvn,Foodenvn,Svind,Hgrp,Ugrp,Tabgrp,Protfak,Fedtfak";
} else {
	print OUT "Foodid,Food_navn,Food_enavn,Svind,Hovedgruppe,Undergruppe,Tabgruppe,Proteinfaktor,Fedtsyrefaktor";
}
foreach (@fieldnos) {print OUT ",$fields{$_}";}
print OUT "\n";

$prevId = 0;
while (<IN>) {
	chop;
	if ($id = 0+substr($_,2,4)) {
		if ($id != $prevId) {
			$prevId = $id;
			@nutrient = (); $nutrient[256] = 0;
			$mgr = $sgr = $tgr = $yld = $ncf = $fcf = 0;
		}
		
		$type = substr($_,6,3);
		if ($type eq 'DAN') {
			$dan{$id} = substr($_,10);
      $dan{$id} =~ tr/‘›†”„‚ ’™Žšµ/æøåöäüéáÆØÅÖÄÜÉÁ/;
			$dan{$id} =~ s/\"/\"\"/g;
		} elsif ($type eq 'OLD') {
			0;
		} elsif ($type eq 'ENG') {
			$eng{$id} = substr($_,10);
			$eng{$id} =~ s/\"/\"\"/g;
		} elsif ($type eq 'SYS') {
			$sys{$id} = substr($_,10);
			$sys{$id} =~ s/\"/\"\"/g;
		} elsif ($type eq 'YLD') {
			$yld = (0.0 + substr($_,10))/100;
		} elsif ($type eq 'MGR') {
			$mgr{$id} = $mgr = 0 + substr($_,10);
		} elsif ($type eq 'SGR') {
			$sgr = 0 + substr($_,10);
			if ($mgr >= 1 && $mgr <= 3) {$tgr = 1;}
			elsif ($mgr == 4) {$tgr = 2;}
			elsif ($mgr == 5 && $sgr == 10) {$tgr = 4;}
			elsif ($mgr == 5 && $sgr == 20) {$tgr = 5;}
			elsif ($mgr == 5 || $mgr == 6) {$tgr = 3;}
			elsif ($mgr >=7 && $mgr <= 11) {$tgr = $mgr-1;}
		} elsif ($type eq 'NCF') {
			$ncf = 0.0 + substr($_,10);
		} elsif ($type eq 'FCF') {
			$fcf = 0.0 + substr($_,10);
		} elsif ($type eq 'END') {
			#print OUT "; $dan{$id} -- $eng{$id}\n";
			if (!defined($zmgr{$mgr})) {$zmgr{$mgr} = {}; print "--$mgr ";}
			$zmgr{$mgr}{$sgr} = 1;
			print OUT "$id,\"$dan{$id}\",\"$eng{$id}\",$yld,$mgr,$sgr,$tgr,$ncf,$fcf";
			foreach (@fieldnos) {print OUT ",$nutrient[$_]";}
			print OUT "\n";
		} elsif ($type eq '150') {
			if ($k = 0.0 + substr($_,10,6)) {$nutrient[150] = $k;} # iflg. Erling Saxholt, Veterinær- og Fødevaredirectoratet
		} elsif ($type eq '004' || $type eq '005' || $type eq '006') {
			0; # iflg. Erling Saxholt, Veterinær- og Fødevaredirectoratet
		} elsif ($fields{0+$type}) {
			$nutrient[$type] = 0.0 + substr($_,10,6);
		} else {
			$unknown{$type}++;
			#print STDERR "Food $id: unknown type: $type\n";
		}
	}
}
close(IN);
close(OUT);

if (%unknown) {
	print STDERR "Ukendte typer i tabellen: ";
	foreach (sort(keys(%unknown))) {print STDERR "$_:$unknown{$_} ";}
	print STDERR "\n";
}

open(F,">indhstof.txt");
if ($kort) {
	print F "
Foodid	Foodid
Foodnvn	Food-navn
Foodenvn	Food-enavn
Svind	Svind
Hgrp	Hovedgruppe
Ugrp	Undergruppe
Tabgrp	Tabgruppe
Protfak	Proteinfaktor
Fedtfak	Fedtsyrefaktor
";
	foreach (@fieldnos) {print F "$fields{$_}\t$fieldsl{$_}\t$_\n";}
} else {
	print F "
Foodid
Food-navn
Food-enavn
Svind
Hovedgruppe
Undergruppe
Tabgruppe
Proteinfaktor
Fedtsyrefaktor
";
	foreach (@fieldnos) {print F "$fields{$_}\t$_\n";}
}
close(F);

open(F,">foodid.txt");
foreach (sort {$a <=> $b;} keys(%dan)) {print F "$_\t$dan{$_}\t$sys{$_}\n";}
close(F);

open(F,">fooddan.txt");
foreach (keys(%dan)) {
	$dan = $dan{$_};
	while ($id{$dan}) {print STDERR "dub $dan\n"; $dan = "$dan x";}
	$id{$dan} = $_;
}
foreach (sort(keys(%id))) {print F "$id{$_}\t$dan{$id{$_}}\t$sys{$id{$_}}\n";}
close(F);

%grptxt = (
	1,	"Konsummælk og mælkekonserves",
	2,	"Ost og osteprodukter",
	3,	"Konsumis",
	4,	"Korn og stivelsesprodukter",
	5,	"Grønsager og grønsagsprodukter",
	6,	"Frugt og frugtprodukter",
	7,	"Kød og kødprodukter",
	8,	"Fisk og fiskeprodukter",
	9,	"Fjerkræ og fjerkræprodukter",
	10,	"Æg og ægprodukter",
	11,	"Fedtstoffer of fedtsofprodukter",
	12,	"Sukker, honning og sukkervarer",
	13,	"Drikkevarer",
	14,	"Krydderier og andre hjælpestoffer",
	15,	"Sammensatte levnedsmidler",
	16,	"Levnedsmidler til særlig ernæring",
	17,	"Andre levnedsmidler",
	20,	"Materialer og husholdningsgenstande",
	21,	"Andre varearter",
);

open(F,">hgrp.txt");
open(G,">ugrp.txt");
if ($kort) {
	print F "hgrp,hgrpnvn\n";
	print G "hgrp,ugrp,ugrpnvn\n";
} else {
	print F "hovedgruppe,hovedgruppe_navn\n";
	print G "hovedgruppe,undergruppe,undergruppe_navn\n";
}
@mgr = sort {$a <=> $b;} keys(%zmgr);
foreach $mgr (@mgr) {
	print F "$mgr,\"$grptxt{$mgr}\"\n";
	@ugr = sort {$a <=> $b;} keys(%{$zmgr{$mgr}});
	foreach $ugr (@ugr) {
		print G "$mgr,$ugr,\"$mgr,$ugr\"\n";
	}
}
close(G);
close(F);


open(F,">foodgrp.txt");
foreach (keys(%dan)) {
	$g = $mgr{$_};
	if (!defined($grp{$g})) {$grp{$g} = {};}
	$dan = $dan{$_};
	while ($grp{$g}{$dan}) {print STDERR "dub2 $dan\n"; $dan = "$dan x";}
	$grp{$g}{$dan} = $_;
}
foreach (sort {$a <=> $b} keys(%grp)) {
	print F "\nGruppe $_:  $grptxt{$_}\n";
	foreach (sort(keys(%{$grp{$_}}))) {print F "$id{$_}\t$dan{$id{$_}}\t$sys{$id{$_}}\n"}
}
close(F);

# Nedenstående er afskrevet fra Tabel 10 i "Levnedsmiddeltabeller, 4.udg. 1996"
if ($kort) {
%tabgrp = (#Tabgrp:  1	 2	 3 	 4 	 5	 6	 7	 8	 9 	10	

		"AvitKo",			[	10,	10,	10,	10,	10,	10,	10,	10,	 5,	15	],
		"AvitSt",			[	10,	10,	10,	10,	10,	20,	20,	20,	20,	50	],
		"AvitBa",			[	10,	10,	10,	10,	10,	 5,	20,	10,	20,	15	],

		"EvitKo",			[	20,	 0,	 0,  0,	 0,	20,	 0,	20,	 0,	25	],
		"EvitSt",			[	20,	 0,	 0,	 0,	 0,	20,	 0,	20,	 0,	80	],
		"EvitBa",			[ 20,	 0,	 0,	 0,	 0, 20,	 0,	20,	 0,	25	],

		"B1vitKo",		[	 0,	15,	25,	25,	40,	60,	20,	60,	10,	 0	],
		"B1vitSt",		[	 0,	20,	25,	25,	40,	20,	20,	20,	15,	 0	],
		"B1vitBa",		[	 0,	20,	25,	25,	40,	20,	30,	20,	15,	 0	],

		"B2vitKo",		[	10,	10,	35,	20,	40,	30,	10,	30,	 5,	 0	],
		"B2vitSt",		[	10,	 5,	35,	30,	40,	20,	20,	20,	10,	 0	],
		"B2vitBa",		[	10,	 5,	35,	30,	40,	20,	20,	20,	20,	 0	],

		"NiacinKo",		[	 0,	10,	35,	30,	40,	50,	15,	50,	 0,	 0	],
		"NiacinSt",		[	 0,	 5,	35,	30,	40,	20,	20,	20,  0,	 0	],
		"NiacinBa",		[	 0,	 5,	35,	30,	40,	20,	20,	20,	 0,	 0	],

		"B6vitKo",		[	10,	40,	40,	40,	40,	50,	 5,	50,	10,	 0	],
		"B6vitSt",		[	10,	40,	40,	40,	40,	20,	20,	20,	20,	 0	],
		"B6vitBa",		[	10,	25,	40,	40,	40,	20,	10,	20,	20,	 0	],

		"B12vitKo",		[	 5,	 0,	 0,	 0,	 0,	20,	 5,	20,	 0,	 0	],
		"B12vitSt",		[	 5,	 0,	 0,	 0,	 0,	20,	 0,	20,	 0,	 0	],
		"B12vitBa",		[	 5,	 0,	 0,	 0,	 0,	20,	10,	20,	 0,	 0	],

		"CvitKo",			[	50,	 0,	50,	40,	70,	20,	20,	20,	 0,	 0	],
		"CvitSt",			[	50,	 0,	50,	40,	70,	20,	20,	20,	 0,	 0	],
		"CvitBa",			[	50,	 0,	50,	40,	70,	20,	20,	20,	 0,	 0	],

		"MineKo",			[	 5,	 5,	 5,	 5,	 5,	 5,	 5,	 5,	 5,	 5	],
		"MineSt",			[	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0	],
		"MineBa",			[	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0	],
	);
} else {
	%tabgrp = (#Tabgruppe: 1	 2	 3 	 4 	 5	 6	 7	 8	 9 	10	
	
		"A_vit_kog",			[	10,	10,	10,	10,	10,	10,	10,	10,	 5,	15	],
		"A_vit_steg",			[	10,	10,	10,	10,	10,	20,	20,	20,	20,	50	],
		"A_vit_bag",			[	10,	10,	10,	10,	10,	 5,	20,	10,	20,	15	],
	
		"E_vit_kog",			[	20,	 0,	 0,  0,	 0,	20,	 0,	20,	 0,	25	],
		"E_vit_steg",			[	20,	 0,	 0,	 0,	 0,	20,	 0,	20,	 0,	80	],
		"E_vit_bag",			[ 20,	 0,	 0,	 0,	 0, 20,	 0,	20,	 0,	25	],
	
		"B1_vit_kog",			[	 0,	15,	25,	25,	40,	60,	20,	60,	10,	 0	],
		"B1_vit_steg",		[	 0,	20,	25,	25,	40,	20,	20,	20,	15,	 0	],
		"B1_vit_bag",			[	 0,	20,	25,	25,	40,	20,	30,	20,	15,	 0	],
	
		"B2_vit_kog",			[	10,	10,	35,	20,	40,	30,	10,	30,	 5,	 0	],
		"B2_vit_steg",		[	10,	 5,	35,	30,	40,	20,	20,	20,	10,	 0	],
		"B2_vit_bag",			[	10,	 5,	35,	30,	40,	20,	20,	20,	20,	 0	],
	
		"Niacin_kog",			[	 0,	10,	35,	30,	40,	50,	15,	50,	 0,	 0	],
		"Niacin_steg",		[	 0,	 5,	35,	30,	40,	20,	20,	20,  0,	 0	],
		"Niacin_bag",			[	 0,	 5,	35,	30,	40,	20,	20,	20,	 0,	 0	],
	
		"B6_vit_kog",			[	10,	40,	40,	40,	40,	50,	 5,	50,	10,	 0	],
		"B6_vit_steg",		[	10,	40,	40,	40,	40,	20,	20,	20,	20,	 0	],
		"B6_vit_bag",			[	10,	25,	40,	40,	40,	20,	10,	20,	20,	 0	],
	
		"B12_vit_kog",		[	 5,	 0,	 0,	 0,	 0,	20,	 5,	20,	 0,	 0	],
		"B12_vit_steg",		[	 5,	 0,	 0,	 0,	 0,	20,	 0,	20,	 0,	 0	],
		"B12_vit_bag",		[	 5,	 0,	 0,	 0,	 0,	20,	10,	20,	 0,	 0	],
	
		"C_vit_kog",			[	50,	 0,	50,	40,	70,	20,	20,	20,	 0,	 0	],
		"C_vit_steg",			[	50,	 0,	50,	40,	70,	20,	20,	20,	 0,	 0	],
		"C_vit_bag",			[	50,	 0,	50,	40,	70,	20,	20,	20,	 0,	 0	],
	
		"Mineral_kog",		[	 5,	 5,	 5,	 5,	 5,	 5,	 5,	 5,	 5,	 5	],
		"Mineral_steg",		[	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0	],
		"Mineral_bag",		[	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0	],
	);
}
open(F,">tabtbl.txt");
@tabf = sort(keys(%tabgrp));
if ($kort) {
	print F "Tabgrp";
} else {
	print F "Tabgruppe";
}
foreach (@tabf) {print F ",$_"}
print F "\n";
$i = 0;
while ($i < 10) {
	print F $i+1;
	foreach (@tabf) {print F ",".($tabgrp{$_}[$i]/100)}
	print F "\n";
	$i++;
}

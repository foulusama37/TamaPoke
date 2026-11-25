// GERADO por tools/gen_textos_golpes_dex.py - nao editar a mao.
// O que faz cada golpe em FR/DE/IT/JA/KO, traduzido do texto do Anil
// (tools/textos_golpes/tr_XX.json). ES/EN/PT estao em moves.h.
#pragma once
#include "moves.h"

// ASSINATURA: o ID de cada linha de MOVE_TBL (a bateria confere)
static const char *const MOVE_DESC_SIG[MOVE_COUNT] = {
  "STRUGGLE",
  "ABSORB",
  "ACCELEROCK",
  "ACID",
  "ACIDARMOR",
  "ACIDSPRAY",
  "ACROBATICS",
  "ACUPRESSURE",
  "AERIALACE",
  "AEROBLAST",
  "AGILITY",
  "AIRCUTTER",
  "AIRSLASH",
  "AMNESIA",
  "ANCHORSHOT",
  "ANCIENTPOWER",
  "APPLEACID",
  "AQUACUTTER",
  "AQUAJET",
  "AQUARING",
  "AQUASTEP",
  "AQUATAIL",
  "ARMORCANNON",
  "ARMTHRUST",
  "AROMATHERAPY",
  "AROMATICMIST",
  "ASSURANCE",
  "ASTONISH",
  "ATRAPAMOSCAS",
  "ATTACKORDER",
  "ATTRACT",
  "AURASPHERE",
  "AURAWHEEL",
  "AURORABEAM",
  "AUTOTOMIZE",
  "AVALANCHE",
  "AXEKICK",
  "BABYDOLLEYES",
  "BADDYBAD",
  "BANEFULBUNKER",
  "BARBBARRAGE",
  "BARRAGE",
  "BARRIER",
  "BATONPASS",
  "BEAKBLAST",
  "BEATUP",
  "BELCH",
  "BELLYDRUM",
  "BIDE",
  "BIND",
  "BITE",
  "BITTERBLADE",
  "BITTERMALICE",
  "BLAZEKICK",
  "BLEAKWINDSTORM",
  "BLIZZARD",
  "BLOCK",
  "BLUEFLARE",
  "BODYPRESS",
  "BODYSLAM",
  "BOLTBEAK",
  "BOLTSTRIKE",
  "BONECLUB",
  "BONEMERANG",
  "BONERUSH",
  "BOOMBURST",
  "BOUNCE",
  "BOUNCYBUBBLE",
  "BRANCHPOKE",
  "BRAVEBIRD",
  "BREAKINGSWIPE",
  "BRICKBREAK",
  "BRINE",
  "BRUTALSWING",
  "BUBBLE",
  "BUBBLEBEAM",
  "BUGBITE",
  "BUGBUZZ",
  "BULKUP",
  "BULLDOZE",
  "BULLETPUNCH",
  "BULLETSEED",
  "BURNINGBULWARK",
  "BURNUP",
  "BUZZYBUZZ",
  "CALMMIND",
  "CAPTIVATE",
  "CHARGE",
  "CHARGEBEAM",
  "CHARM",
  "CHATTER",
  "CHILLINGWATER",
  "CHILLYRECEPTION",
  "CHIPAWAY",
  "CHLOROBLAST",
  "CIRCLETHROW",
  "CLAMP",
  "CLANGINGSCALES",
  "CLANGOROUSSOUL",
  "CLEARSMOG",
  "CLOSECOMBAT",
  "COIL",
  "COLLISIONCOURSE",
  "COMETPUNCH",
  "COMEUPPANCE",
  "CONFIDE",
  "CONFUSERAY",
  "CONFUSION",
  "CONSTRICT",
  "COPYCAT",
  "COREENFORCER",
  "COSMICPOWER",
  "COTTONGUARD",
  "COTTONSPORE",
  "COUNTER",
  "COVET",
  "CRABHAMMER",
  "CROSSCHOP",
  "CROSSPOISON",
  "CRUNCH",
  "CRUSHCLAW",
  "CRUSHGRIP",
  "CURSE",
  "CUT",
  "DARKESTLARIAT",
  "DARKPULSE",
  "DARKVOID",
  "DAZZLINGGLEAM",
  "DEFENDORDER",
  "DEFENSECURL",
  "DEFOG",
  "DESLIZAMIENTO",
  "DESTINYBOND",
  "DETECT",
  "DIAMONDSTORM",
  "DIG",
  "DIRECLAW",
  "DISABLE",
  "DISARMINGVOICE",
  "DISCHARGE",
  "DIVE",
  "DIZZYPUNCH",
  "DOOMDESIRE",
  "DOUBLEEDGE",
  "DOUBLEHIT",
  "DOUBLEIRONBASH",
  "DOUBLEKICK",
  "DOUBLESHOCK",
  "DOUBLESLAP",
  "DOUBLETEAM",
  "DRACOMETEOR",
  "DRAGONASCENT",
  "DRAGONBREATH",
  "DRAGONCLAW",
  "DRAGONDANCE",
  "DRAGONDARTS",
  "DRAGONENERGY",
  "DRAGONHAMMER",
  "DRAGONPULSE",
  "DRAGONRAGE",
  "DRAGONRUSH",
  "DRAGONTAIL",
  "DRAININGKISS",
  "DRAINPUNCH",
  "DREAMEATER",
  "DRILLPECK",
  "DRILLRUN",
  "DRUMBEATING",
  "DUALCHOP",
  "DUALWINGBEAT",
  "DYNAMAXCANNON",
  "DYNAMICPUNCH",
  "EARTHPOWER",
  "EARTHQUAKE",
  "ECHOEDVOICE",
  "EERIEIMPULSE",
  "EGGBOMB",
  "ELECTRICTERRAIN",
  "ELECTROBALL",
  "ELECTRODRIFT",
  "ELECTROSHOT",
  "ELECTROWEB",
  "EMBER",
  "ENCORE",
  "ENDEAVOR",
  "ENDURE",
  "ENERGYBALL",
  "ENTRAINMENT",
  "ERUPTION",
  "ESCALOFRIO",
  "ESPERWING",
  "ETERNABEAM",
  "EXPANDINGFORCE",
  "EXPLOSION",
  "EXTRASENSORY",
  "EXTREMESPEED",
  "FACADE",
  "FAIRYWIND",
  "FAKEOUT",
  "FAKETEARS",
  "FALSESURRENDER",
  "FALSESWIPE",
  "FEATHERDANCE",
  "FEINT",
  "FEINTATTACK",
  "FELLSTINGER",
  "FICKLEBEAM",
  "FIERYDANCE",
  "FINALGAMBIT",
  "FIREBLAST",
  "FIREFANG",
  "FIRELASH",
  "FIREPUNCH",
  "FIRESPIN",
  "FIRSTIMPRESSION",
  "FISHIOUSREND",
  "FISSURE",
  "FLAIL",
  "FLAMEBURST",
  "FLAMECHARGE",
  "FLAMETHROWER",
  "FLAMEWHEEL",
  "FLAREBLITZ",
  "FLASH",
  "FLASHCANNON",
  "FLATTER",
  "FLEURCANNON",
  "FLING",
  "FLIPTURN",
  "FLOWERTRICK",
  "FLY",
  "FLYINGPRESS",
  "FOCUSBLAST",
  "FOCUSENERGY",
  "FOCUSPUNCH",
  "FORCEPALM",
  "FORESIGHT",
  "FOULPLAY",
  "FREEZEDRY",
  "FREEZYFROST",
  "FROSTBREATH",
  "FRUSTRATION",
  "FURYATTACK",
  "FURYCUTTER",
  "FURYSWIPES",
  "FUSIONBOLT",
  "FUSIONFLARE",
  "FUTURESIGHT",
  "GASTROACID",
  "GEARGRIND",
  "GEOMANCY",
  "GIGADRAIN",
  "GIGAIMPACT",
  "GIGATONHAMMER",
  "GLACIATE",
  "GLAIVERUSH",
  "GLARE",
  "GLITZYGLOW",
  "GRASSKNOT",
  "GRASSWHISTLE",
  "GRASSYGLIDE",
  "GRASSYTERRAIN",
  "GRAVAPPLE",
  "GROWL",
  "GROWTH",
  "GRUDGE",
  "GUILLOTINE",
  "GUNKSHOT",
  "GUST",
  "GYROBALL",
  "HAMMERARM",
  "HARDEN",
  "HAZE",
  "HEADBUTT",
  "HEADCHARGE",
  "HEADLONGRUSH",
  "HEADSMASH",
  "HEALBELL",
  "HEALINGWISH",
  "HEALORDER",
  "HEALPULSE",
  "HEARTSTAMP",
  "HEATCRASH",
  "HEATWAVE",
  "HEAVYSLAM",
  "HEX",
  "HIDDENPOWER",
  "HIGHHORSEPOWER",
  "HIGHJUMPKICK",
  "HONECLAWS",
  "HORNATTACK",
  "HORNDRILL",
  "HORNLEECH",
  "HOWL",
  "HURRICANE",
  "HYDROPUMP",
  "HYDROSTEAM",
  "HYPERBEAM",
  "HYPERDRILL",
  "HYPERFANG",
  "HYPERSPACEHOLE",
  "HYPERVOICE",
  "HYPNOSIS",
  "ICEBALL",
  "ICEBEAM",
  "ICEFANG",
  "ICEHAMMER",
  "ICEPUNCH",
  "ICESHARD",
  "ICESPINNER",
  "ICICLECRASH",
  "ICICLESPEAR",
  "ICYWIND",
  "IMPRISON",
  "INCINERATE",
  "INFERNO",
  "INFESTATION",
  "INGRAIN",
  "IONDELUGE",
  "IRONDEFENSE",
  "IRONHEAD",
  "IRONTAIL",
  "IVYCUDGEL",
  "JAWLOCK",
  "JETPUNCH",
  "JUDGMENT",
  "JUMPKICK",
  "JUNGLEHEALING",
  "KARATECHOP",
  "KINESIS",
  "KINGSSHIELD",
  "KNOCKOFF",
  "KOWTOWCLEAVE",
  "LANDSWRATH",
  "LASERFOCUS",
  "LASTRESORT",
  "LASTRESPECTS",
  "LAVAPLUME",
  "LEAFAGE",
  "LEAFBLADE",
  "LEAFSTORM",
  "LEAFTORNADO",
  "LEECHLIFE",
  "LEECHSEED",
  "LEER",
  "LICK",
  "LIFEDEW",
  "LIGHTSCREEN",
  "LIQUIDATION",
  "LOCKON",
  "LOVELYKISS",
  "LOWKICK",
  "LOWSWEEP",
  "LUCKYCHANT",
  "LUMINACRASH",
  "LUNARBLESSING",
  "LUNARDANCE",
  "LUNGE",
  "LUSTERPURGE",
  "MACHPUNCH",
  "MAGICALLEAF",
  "MAGMASTORM",
  "MAGNETBOMB",
  "MAGNETRISE",
  "MAGNITUDE",
  "MAKEITRAIN",
  "MALIGNANTCHAIN",
  "MATCHAGOTCHA",
  "MEANLOOK",
  "MEDITATE",
  "MEGADRAIN",
  "MEGAHORN",
  "MEGAKICK",
  "MEGAPUNCH",
  "MEMENTO",
  "METALBURST",
  "METALCLAW",
  "METALSOUND",
  "METEORASSAULT",
  "METEORBEAM",
  "METEORMASH",
  "MIGHTYCLEAVE",
  "MILKDRINK",
  "MINDBLOWN",
  "MINDREADER",
  "MINIMIZE",
  "MIRACLEEYE",
  "MIRRORCOAT",
  "MIRRORMOVE",
  "MIRRORSHOT",
  "MIST",
  "MISTBALL",
  "MISTYEXPLOSION",
  "MISTYTERRAIN",
  "MOONBLAST",
  "MOONGEISTBEAM",
  "MOONLIGHT",
  "MORNINGSUN",
  "MORTALSPIN",
  "MOUNTAINGALE",
  "MUDBOMB",
  "MUDDYWATER",
  "MUDSHOT",
  "MUDSLAP",
  "MUDSPORT",
  "MULTIATTACK",
  "MYSTICALFIRE",
  "MYSTICALPOWER",
  "NASTYPLOT",
  "NATURALGIFT",
  "NATURESMADNESS",
  "NEEDLEARM",
  "NIGHTDAZE",
  "NIGHTSHADE",
  "NIGHTSLASH",
  "NOBLEROAR",
  "NORETREAT",
  "NUZZLE",
  "OBLIVIONWING",
  "OBSTRUCT",
  "OCTAZOOKA",
  "OCTOLOCK",
  "ODORSLEUTH",
  "OMINOUSWIND",
  "ORDERUP",
  "ORIGINPULSE",
  "OUTRAGE",
  "OVERDRIVE",
  "OVERHEAT",
  "PAINSPLIT",
  "PARABOLICCHARGE",
  "PARTINGSHOT",
  "PAYBACK",
  "PAYDAY",
  "PECK",
  "PETALBLIZZARD",
  "PETALDANCE",
  "PHANTOMFORCE",
  "PHOTONGEYSER",
  "PINMISSILE",
  "PLASMAFISTS",
  "PLAYNICE",
  "PLAYROUGH",
  "PLUCK",
  "POISONFANG",
  "POISONGAS",
  "POISONJAB",
  "POISONPOWDER",
  "POISONSTING",
  "POISONTAIL",
  "POLLENPUFF",
  "POLTERGEIST",
  "POPULATIONBOMB",
  "POUNCE",
  "POUND",
  "POWDERSNOW",
  "POWERGEM",
  "POWERTRIP",
  "POWERUPPUNCH",
  "POWERWHIP",
  "PRECIPICEBLADES",
  "PRESENT",
  "PRISMATICLASER",
  "PROTECT",
  "PSYBEAM",
  "PSYBLADE",
  "PSYCHIC",
  "PSYCHICFANGS",
  "PSYCHICTERRAIN",
  "PSYCHOBOOST",
  "PSYCHOCUT",
  "PSYCHUP",
  "PSYSHIELDBASH",
  "PSYSHOCK",
  "PSYSTRIKE",
  "PSYWAVE",
  "PUNISHMENT",
  "PURIFY",
  "PURSUIT",
  "PYROBALL",
  "QUICKATTACK",
  "QUICKGUARD",
  "QUIVERDANCE",
  "RAGE",
  "RAGEFIST",
  "RAGINGBULL",
  "RAGINGFURY",
  "RAINDANCE",
  "RAPIDSPIN",
  "RAZORLEAF",
  "RAZORSHELL",
  "RAZORWIND",
  "RECOVER",
  "RECYCLE",
  "REFLECT",
  "REFRESH",
  "RELICSONG",
  "REST",
  "RETALIATE",
  "RETURN",
  "REVELATIONDANCE",
  "REVENGE",
  "REVERSAL",
  "RISINGVOLTAGE",
  "ROAR",
  "ROAROFTIME",
  "ROCKBLAST",
  "ROCKCLIMB",
  "ROCKPOLISH",
  "ROCKSLIDE",
  "ROCKSMASH",
  "ROCKTHROW",
  "ROCKTOMB",
  "ROCKWRECKER",
  "ROLLINGKICK",
  "ROLLOUT",
  "ROOST",
  "ROUND",
  "RUINATION",
  "SACREDFIRE",
  "SACREDSWORD",
  "SAFEGUARD",
  "SALTCURE",
  "SANDATTACK",
  "SANDSEARSTORM",
  "SANDSTORM",
  "SANDTOMB",
  "SAPPYSEED",
  "SCALD",
  "SCALESHOT",
  "SCARYFACE",
  "SCORCHINGSANDS",
  "SCRATCH",
  "SCREECH",
  "SEARINGSHOT",
  "SECRETPOWER",
  "SECRETSWORD",
  "SEEDBOMB",
  "SEEDFLARE",
  "SEISMICTOSS",
  "SELFDESTRUCT",
  "SHADOWBALL",
  "SHADOWBONE",
  "SHADOWCLAW",
  "SHADOWFORCE",
  "SHADOWPUNCH",
  "SHADOWSNEAK",
  "SHARPEN",
  "SHEERCOLD",
  "SHELLSMASH",
  "SHELLTRAP",
  "SHOCKWAVE",
  "SHOREUP",
  "SIGNALBEAM",
  "SILKTRAP",
  "SILVERWIND",
  "SING",
  "SIZZLYSLIDE",
  "SKETCH",
  "SKITTERSMACK",
  "SKULLBASH",
  "SKYATTACK",
  "SKYUPPERCUT",
  "SLACKOFF",
  "SLAM",
  "SLASH",
  "SLEEPPOWDER",
  "SLUDGE",
  "SLUDGEBOMB",
  "SLUDGEWAVE",
  "SMACKDOWN",
  "SMARTSTRIKE",
  "SMELLINGSALTS",
  "SMOG",
  "SMOKESCREEN",
  "SNAPTRAP",
  "SNARL",
  "SNIPESHOT",
  "SNORE",
  "SNOWSCAPE",
  "SOAK",
  "SOFTBOILED",
  "SOLARBEAM",
  "SOLARBLADE",
  "SONICBOOM",
  "SPACIALREND",
  "SPARK",
  "SPARKLINGARIA",
  "SPARKLYSWIRL",
  "SPECTRALTHIEF",
  "SPIDERWEB",
  "SPIKECANNON",
  "SPIKES",
  "SPIKYSHIELD",
  "SPINOUT",
  "SPIRITBREAK",
  "SPIRITSHACKLE",
  "SPITE",
  "SPITUP",
  "SPLASH",
  "SPORE",
  "SPRINGTIDESTORM",
  "STEALTHROCK",
  "STEAMERUPTION",
  "STEAMROLLER",
  "STEELBEAM",
  "STEELROLLER",
  "STEELWING",
  "STICKYWEB",
  "STOCKPILE",
  "STOMP",
  "STOMPINGTANTRUM",
  "STONEAXE",
  "STONEEDGE",
  "STOREDPOWER",
  "STORMTHROW",
  "STRANGESTEAM",
  "STRENGTH",
  "STRENGTHSAP",
  "STRINGSHOT",
  "STRUGGLEBUG",
  "STUNSPORE",
  "SUBMISSION",
  "SUCKERPUNCH",
  "SUNNYDAY",
  "SUNSTEELSTRIKE",
  "SUPERCELLSLAM",
  "SUPERFANG",
  "SUPERPOWER",
  "SUPERSONIC",
  "SURF",
  "SWAGGER",
  "SWALLOW",
  "SWEETKISS",
  "SWEETSCENT",
  "SWIFT",
  "SWITCHEROO",
  "SWORDSDANCE",
  "SYNCHRONOISE",
  "SYNTHESIS",
  "SYRUPBOMB",
  "TACHYONCUTTER",
  "TACKLE",
  "TAILGLOW",
  "TAILSLAP",
  "TAILWHIP",
  "TAILWIND",
  "TAKEDOWN",
  "TAKEHEART",
  "TARSHOT",
  "TAUNT",
  "TEARFULLOOK",
  "TECHNOBLAST",
  "TEETERDANCE",
  "TERASTARSTORM",
  "TERRAINPULSE",
  "THIEF",
  "THOUSANDARROWS",
  "THOUSANDWAVES",
  "THRASH",
  "THROATCHOP",
  "THUNDER",
  "THUNDERBOLT",
  "THUNDERCAGE",
  "THUNDERCLAP",
  "THUNDERFANG",
  "THUNDERPUNCH",
  "THUNDERSHOCK",
  "THUNDERWAVE",
  "TICKLE",
  "TIDYUP",
  "TORCHSONG",
  "TORMENT",
  "TOXIC",
  "TOXICSPIKES",
  "TOXICTHREAD",
  "TRANSFORM",
  "TRIATTACK",
  "TRICK",
  "TRIPLEDIVE",
  "TRIPLEHIT",
  "TRIPLEKICK",
  "TROPKICK",
  "TRUMPCARD",
  "TWINBEAM",
  "TWINEEDLE",
  "TWISTER",
  "UPROAR",
  "UTURN",
  "VACUUMWAVE",
  "VCREATE",
  "VENOMDRENCH",
  "VENOSHOCK",
  "VINEWHIP",
  "VISEGRIP",
  "VITALTHROW",
  "VOLTSWITCH",
  "VOLTTACKLE",
  "WAKEUPSLAP",
  "WATERFALL",
  "WATERGUN",
  "WATERPULSE",
  "WATERSHURIKEN",
  "WATERSPORT",
  "WATERSPOUT",
  "WAVECRASH",
  "WEATHERBALL",
  "WHIRLPOOL",
  "WHIRLWIND",
  "WICKEDBLOW",
  "WILDBOLTSTORM",
  "WILDCHARGE",
  "WILLOWISP",
  "WINGATTACK",
  "WITHDRAW",
  "WOODHAMMER",
  "WORKUP",
  "WORRYSEED",
  "WRAP",
  "WRINGOUT",
  "XSCISSOR",
  "YAWN",
  "ZAPCANNON",
  "ZENHEADBUTT",
  "ZINGZAP",
  "EMBARGO",
  "HEALBLOCK",
  "PERISHSONG",
  "SUBSTITUTE",
  "TRICKROOM",
  "WISH",
};

static const char *const MOVE_DESC_FR[MOVE_COUNT] = {
  "Utilis\202e quand il ne reste plus de capacit\202s. Blesse aussi le lanceur.",  // STRUGGLE
  "Absorbe la moiti\202 des d\202g\203ts inflig\202s pour restaurer les PV du lanceur.",  // ABSORB
  "Le lanceur percute la cible \205 grande vitesse. Frappe toujours en premier.",  // ACCELEROCK
  "Asperge la cible d'un acide corrosif. Peut baisser sa D\202f. Sp\202.",  // ACID
  "Le lanceur liqu\202fie son corps, ce qui augmente beaucoup sa D\202fense.",  // ACIDARMOR
  "Attaque avec un liquide corrosif qui baisse beaucoup la D\202f. Sp\202. de la cible.",  // ACIDSPRAY
  "Frappe avec agilit\202. Inflige d'\202normes d\202g\203ts si le lanceur ne tient pas d'objet.",  // ACROBATICS
  "Une pression sur des points cl\202s du corps augmente beaucoup une stat au hasard.",  // ACUPRESSURE
  "Le lanceur porte une attaque tr\212s rapide impossible \205 esquiver.",  // AERIALACE
  "Projette une rafale d'air. Taux de critique \202lev\202.",  // AEROBLAST
  "Le lanceur se d\202tend, ce qui augmente beaucoup sa Vitesse.",  // AGILITY
  "Un vent tranchant fouette la cible. Taux de critique \202lev\202.",  // AIRCUTTER
  "Une lame d'air qui fend le ciel. Peut apeurer la cible.",  // AIRSLASH
  "Le lanceur oublie ses soucis, ce qui augmente beaucoup sa D\202f. Sp\202.",  // AMNESIA
  "Lance une ancre sur la cible, qui est pi\202g\202e et ne peut plus fuir.",  // ANCHORSHOT
  "Une attaque pr\202historique. Peut augmenter toutes les stats du lanceur.",  // ANCIENTPOWER
  "Attaque avec le jus corrosif d'une pomme acide. Baisse la D\202f. Sp\202. de la cible.",  // APPLEACID
  "Tranche la cible avec de l'eau sous pression, comme une lame. Taux de critique \202lev\202.",  // AQUACUTTER
  "Une attaque rapide qui frappe toujours en premier.",  // AQUAJET
  "Un voile d'eau entoure le lanceur. Il r\202cup\212re des PV \205 chaque tour.",  // AQUARING
  "Une danse \202l\202gante et fluide qui inflige des d\202g\203ts. Augmente la Vitesse du lanceur.",  // AQUASTEP
  "Le lanceur frappe en agitant sa queue comme une vague d\202cha\214n\202e dans une temp\210te.",  // AQUATAIL
  "Projette des morceaux br\226lants de son armure. Baisse la D\202fense et la D\202f. Sp\202. du lanceur.",  // ARMORCANNON
  "Des coups de paume directs qui frappent 2 \205 5 fois de suite.",  // ARMTHRUST
  "Un doux parfum soigne les probl\212mes de statut de l'\202quipe.",  // AROMATHERAPY
  "Un parfum myst\202rieux augmente la D\202f. Sp\202. d'un Pok\202mon de l'\202quipe.",  // AROMATICMIST
  "Si la cible a d\202j\205 subi des d\202g\203ts ce tour-ci, la puissance double.",  // ASSURANCE
  "Une attaque si surprenante qu'elle peut apeurer la cible.",  // ASTONISH
  "Mord avec des m\203choires tr\212s efficaces contre les Pok\202mon de type Insecte.",  // ATRAPAMOSCAS
  "Le lanceur appelle ses sbires pour frapper la cible. Taux de critique \202lev\202.",  // ATTACKORDER
  "Si la cible est de sexe oppos\202, elle tombe amoureuse et attaque moins souvent.",  // ATTRACT
  "Le lanceur lib\212re une rafale d'aura. N'\202choue jamais.",  // AURASPHERE
  "Attaque et augmente la Vitesse du lanceur. Son type change selon la forme du lanceur.",  // AURAWHEEL
  "Un rayon multicolore qui peut baisser l'Attaque de la cible.",  // AURORABEAM
  "Le lanceur se d\202barrasse d'une partie de son corps et augmente beaucoup sa Vitesse.",  // AUTOTOMIZE
  "Inflige le double de d\202g\203ts \205 une cible qui a frapp\202 le lanceur ce tour-ci.",  // AVALANCHE
  "Un coup de talon venu d'en haut. Si elle \202choue, le lanceur se blesse. Peut rendre confus.",  // AXEKICK
  "Le lanceur fixe la cible, ce qui baisse son Attaque. Agit toujours en premier.",  // BABYDOLLEYES
  "Le lanceur fait le m\202chant, attaque la cible et met en place Protection.",  // BADDYBAD
  "Prot\212ge des attaques et empoisonne tout ennemi qui touche le lanceur.",  // BANEFULBUNKER
  "Tire des pointes toxiques qui peuvent empoisonner. Puissance doubl\202e si la cible est empoisonn\202e.",  // BARBBARRAGE
  "Lance des objets ronds sur la cible, qui frappent 2 \205 5 fois de suite.",  // BARRAGE
  "Cr\202e une barri\212re solide qui augmente beaucoup la D\202fense.",  // BARRIER
  "Le lanceur \202change sa place avec un alli\202 et lui transmet ses changements de stats.",  // BATONPASS
  "Chauffe son bec avant d'attaquer. Br\226le l'ennemi qui le frappe au contact pendant la charge.",  // BEAKBLAST
  "Toute l'\202quipe participe \205 l'attaque. Plus il y a d'alli\202s, plus il y a de coups.",  // BEATUP
  "Inflige des d\202g\203ts en rotant. Le lanceur doit d'abord manger une Baie qu'il tient.",  // BELCH
  "Le lanceur sacrifie la moiti\202 de ses PV max pour monter son Attaque au maximum.",  // BELLYDRUM
  "Encaisse les coups pendant 2 tours, puis riposte avec le double des d\202g\203ts subis.",  // BIDE
  "Ligote et serre la cible pendant 4 \205 5 tours.",  // BIND
  "Une morsure vorace qui peut apeurer la cible.",  // BITE
  "Un coup d'\202p\202e plein de rancoeur. Le lanceur r\202cup\212re la moiti\202 des d\202g\203ts inflig\202s.",  // BITTERBLADE
  "Attaque la cible avec une froide rancoeur et baisse son Attaque.",  // BITTERMALICE
  "Un coup de pied au taux de critique \202lev\202. Peut br\226ler la cible.",  // BLAZEKICK
  "Un vent glacial qui fait frissonner. Peut baisser la Vitesse de la cible.",  // BLEAKWINDSTORM
  "Une temp\210te de glace qui peut geler la cible.",  // BLIZZARD
  "Le lanceur bloque le chemin de la cible pour l'emp\210cher de fuir.",  // BLOCK
  "Enveloppe la cible d'une flamme bleue aussi belle que puissante. Peut la br\226ler.",  // BLUEFLARE
  "Le lanceur charge la cible. Plus sa D\202fense est \202lev\202e, plus les d\202g\203ts sont importants.",  // BODYPRESS
  "Le lanceur se jette de tout son poids sur la cible. Peut la paralyser.",  // BODYSLAM
  "Attaque avec un bec \202lectrifi\202. Puissance doubl\202e si le lanceur agit avant la cible.",  // BOLTBEAK
  "Le lanceur s'entoure d'\202lectricit\202 et charge la cible. Peut la paralyser.",  // BOLTSTRIKE
  "Frappe la cible avec un os. Peut l'apeurer.",  // BONECLUB
  "Lance un os comme un boomerang. Frappe 2 fois.",  // BONEMERANG
  "Frappe la cible avec un os 2 \205 5 fois de suite.",  // BONERUSH
  "Attaque tous les Pok\202mon autour du lanceur avec une puissante onde sonore.",  // BOOMBURST
  "Bondit au 1er tour et frappe au 2e. Peut paralyser la cible.",  // BOUNCE
  "Tire des bulles d'eau sur la cible et absorbe la moiti\202 des d\202g\203ts inflig\202s.",  // BOUNCYBUBBLE
  "Pique la cible avec une branche pointue.",  // BRANCHPOKE
  "Le lanceur replie ses ailes et fonce en rase-mottes. Il subit aussi des d\202g\203ts.",  // BRAVEBIRD
  "Fouette violemment la cible avec sa queue. Baisse l'Attaque de la cible.",  // BREAKINGSWIPE
  "Une attaque puissante qui brise aussi les barri\212res comme Mur Lumi\212re et Protection.",  // BRICKBREAK
  "Puissance doubl\202e si la cible a la moiti\202 de ses PV ou moins.",  // BRINE
  "Le lanceur fait tournoyer son corps et blesse tous les ennemis.",  // BRUTALSWING
  "Projette des bulles sur l'ennemi. Peut baisser sa Vitesse.",  // BUBBLE
  "Un d\202luge de bulles qui peut baisser la Vitesse.",  // BUBBLEBEAM
  "Pique la cible. Si elle tient une Baie, le lanceur la mange et profite de son effet.",  // BUGBITE
  "Le lanceur fait vibrer ses ailes et cr\202e une onde sonore. Peut baisser la D\202f. Sp\202.",  // BUGBUZZ
  "Le lanceur gonfle ses muscles pour augmenter son Attaque et sa D\202fense.",  // BULKUP
  "Une secousse sismique frappe les Pok\202mon voisins et baisse leur Vitesse.",  // BULLDOZE
  "Un poing aussi rapide qu'une balle. Frappe toujours en premier.",  // BULLETPUNCH
  "Tire rapidement des rafales de graines 2 \205 5 fois de suite.",  // BULLETSEED
  "Le lanceur se prot\212ge des attaques avec sa fourrure. Br\226le l'ennemi qui le touche.",  // BURNINGBULWARK
  "Le lanceur consume toutes ses flammes pour attaquer, mais perd le type Feu.",  // BURNUP
  "Le lanceur tire une d\202charge \202lectrique sur la cible. Paralyse toujours.",  // BUZZYBUZZ
  "Le lanceur se concentre et s'apaise, ce qui augmente son Atq. Sp\202. et sa D\202f. Sp\202.",  // CALMMIND
  "Si la cible est de sexe oppos\202, elle est charm\202e et son Atq. Sp\202. baisse beaucoup.",  // CAPTIVATE
  "Accumule de l'\202nergie pour renforcer la prochaine capacit\202 \220lectrik. Augmente la D\202f. Sp\202.",  // CHARGE
  "Lance un rayon \202lectrique sur l'ennemi. Peut augmenter l'Atq. Sp\202. du lanceur.",  // CHARGEBEAM
  "Le lanceur charme la cible, ce qui baisse beaucoup son Attaque.",  // CHARM
  "Attaque avec une onde sonore faite de mots appris. Peut rendre la cible confuse.",  // CHATTER
  "Asperge la cible d'une eau glac\202e et d\202moralisante qui baisse son Attaque.",  // CHILLINGWATER
  "Raconte une blague si glaciale qu'il se met \205 neiger, puis le lanceur est remplac\202.",  // CHILLYRECEPTION
  "Vise les points faibles de la cible en ignorant ses changements de stats.",  // CHIPAWAY
  "Concentre sa chlorophylle et la tire en rayon, mais le lanceur se blesse aussi.",  // CHLOROBLAST
  "Projette la cible et la force \205 \210tre remplac\202e. Contre un Pok\202mon sauvage, finit le combat.",  // CIRCLETHROW
  "La cible est coinc\202e et serr\202e pendant 4 \205 5 tours.",  // CLAMP
  "Frotte ses \202cailles pour cr\202er un vacarme, mais baisse la D\202fense du lanceur.",  // CLANGINGSCALES
  "Le lanceur augmente toutes ses stats en \202change d'une partie de ses PV.",  // CLANGOROUSSOUL
  "Attaque avec une \202trange brume qui annule les changements de stats de la cible.",  // CLEARSMOG
  "Le lanceur se bat au corps \205 corps. Baisse sa D\202fense et sa D\202f. Sp\202.",  // CLOSECOMBAT
  "Le lanceur se concentre et augmente son Attaque, sa D\202fense et sa Pr\202cision.",  // COIL
  "Percute le sol en se transformant. Plus puissant si l'attaque est super efficace.",  // COLLISIONCOURSE
  "Une rafale de coups de poing qui frappe 2 \205 5 fois de suite.",  // COMETPUNCH
  "Renvoie \205 l'ennemi la derni\212re attaque subie, avec beaucoup plus de force.",  // COMEUPPANCE
  "Distrait l'ennemi en lui confiant un secret, ce qui baisse son Atq. Sp\202.",  // CONFIDE
  "Un rayon sinistre qui rend la cible confuse.",  // CONFUSERAY
  "Une faible force t\202l\202kin\202tique. Peut rendre la cible confuse.",  // CONFUSION
  "Serre la cible pour la blesser. Peut baisser sa Vitesse.",  // CONSTRICT
  "Imite la capacit\202 utilis\202e juste avant. \220choue si aucune n'a encore \202t\202 utilis\202e.",  // COPYCAT
  "Blesse l'ennemi. S'il a d\202j\205 utilis\202 une capacit\202 ce tour-ci, son talent est annul\202.",  // COREENFORCER
  "Une \202nergie mystique augmente la D\202fense et la D\202f. Sp\202. du lanceur.",  // COSMICPOWER
  "Le lanceur s'entoure de coton protecteur. Augmente \202norm\202ment sa D\202fense.",  // COTTONGUARD
  "Des spores collent \205 la cible et baissent beaucoup sa Vitesse.",  // COTTONSPORE
  "Riposte \205 une attaque physique en infligeant le double des d\202g\203ts subis.",  // COUNTER
  "S'approche tendrement de la cible, puis l'attaque et peut voler son objet.",  // COVET
  "Frappe violemment avec une grosse pince. Taux de critique \202lev\202.",  // CRABHAMMER
  "Une double manchette. Taux de critique \202lev\202.",  // CROSSCHOP
  "Un coup tranchant qui peut empoisonner la cible. Taux de critique \202lev\202.",  // CROSSPOISON
  "Broie la cible avec des crocs ac\202r\202s. Peut baisser sa D\202fense.",  // CRUNCH
  "Lac\212re la cible avec des griffes ac\202r\202es. Peut baisser sa D\202fense.",  // CRUSHCLAW
  "Plus il reste de PV \205 la cible, plus l'attaque est puissante.",  // CRUSHGRIP
  "Une capacit\202 dont l'effet change selon que le lanceur est de type Spectre ou non.",  // CURSE
  "Tranche avec des griffes, des faux, etc. Sert aussi \205 couper les petits arbres.",  // CUT
  "Le lanceur tournoie et frappe la cible en ignorant ses changements de stats.",  // DARKESTLARIAT
  "Lib\212re une aura horrible charg\202e d'id\202es noires. Peut apeurer la cible.",  // DARKPULSE
  "La cible est entra\214n\202e dans un monde de t\202n\212bres qui l'endort.",  // DARKVOID
  "Blesse les ennemis avec une lumi\212re puissante.",  // DAZZLINGGLEAM
  "Le lanceur appelle ses sbires pour former un bouclier vivant. Augmente D\202fense et D\202f. Sp\202.",  // DEFENDORDER
  "Le lanceur s'enroule pour cacher ses points faibles. Augmente sa D\202fense.",  // DEFENSECURL
  "Un vent puissant qui balaie Protection ou Mur Lumi\212re de la cible. Peut baisser son Esquive.",  // DEFOG
  "Le lanceur glisse sur la neige pour attaquer, ce qui augmente sa Vitesse.",  // DESLIZAMIENTO
  "Si le lanceur est mis K.O., l'ennemi qui l'a battu est aussi mis K.O.",  // DESTINYBOND
  "Bloque les attaques, mais peut \202chouer si utilis\202e plusieurs fois de suite.",  // DETECT
  "Une temp\210te d\202vastatrice de diamants. Peut augmenter la D\202fense du lanceur.",  // DIAMONDSTORM
  "Creuse au 1er tour, attaque au 2e. Permet aussi de sortir de certains lieux.",  // DIG
  "Frappe le point faible de la cible avec des griffes funestes. Peut empoisonner, paralyser ou endormir.",  // DIRECLAW
  "Emp\210che la cible d'utiliser sa derni\212re capacit\202 pendant 4 tours.",  // DISABLE
  "Le lanceur charme les ennemis de sa voix et les blesse \202motionnellement. N'\202choue jamais.",  // DISARMINGVOICE
  "Une onde \202lectrique \202blouissante frappe les autres Pok\202mon. Peut paralyser.",  // DISCHARGE
  "Plonge sous l'eau au 1er tour, puis surgit et attaque au 2e.",  // DIVE
  "Des coups de poing rythm\202s qui peuvent rendre la cible confuse.",  // DIZZYPUNCH
  "Concentre un faisceau de lumi\212re qui frappe la cible 2 tours plus tard.",  // DOOMDESIRE
  "Une charge t\202m\202raire qui blesse aussi le lanceur.",  // DOUBLEEDGE
  "Frappe la cible 2 fois de suite avec sa queue ou une autre partie du corps.",  // DOUBLEHIT
  "Frappe 2 fois de suite avec ses poings. Peut apeurer la cible.",  // DOUBLEIRONBASH
  "Un double coup de pied qui frappe 2 fois.",  // DOUBLEKICK
  "Lib\212re toute l'\202lectricit\202 de son corps pour attaquer. Le lanceur perd le type \220lectrik.",  // DOUBLESHOCK
  "Gifle la cible 2 \205 5 fois de suite.",  // DOUBLESLAP
  "Le lanceur cr\202e des copies de lui-m\210me pour augmenter son Esquive.",  // DOUBLETEAM
  "Fait tomber de grandes com\212tes du ciel. Baisse beaucoup l'Atq. Sp\202. du lanceur.",  // DRACOMETEOR
  "Fond du ciel \205 grande vitesse, mais baisse la D\202fense et la D\202f. Sp\202. du lanceur.",  // DRAGONASCENT
  "Un souffle puissant qui frappe la cible. Peut la paralyser.",  // DRAGONBREATH
  "Lac\212re la cible avec des griffes ac\202r\202es.",  // DRAGONCLAW
  "Une danse mystique qui augmente l'Attaque et la Vitesse.",  // DRAGONDANCE
  "Attaque 2 fois avec des Fantyrm. S'il y a 2 ennemis, frappe chacun une fois.",  // DRAGONDARTS
  "Attaque avec sa force vitale. Moins le lanceur a de PV, moins l'attaque est puissante.",  // DRAGONENERGY
  "Le lanceur se sert de son corps comme d'un marteau pour frapper la cible.",  // DRAGONHAMMER
  "Ouvre grand la gueule et lib\212re une onde de choc sur la cible.",  // DRAGONPULSE
  "Une onde de choc furieuse qui inflige toujours 40 PV de d\202g\203ts.",  // DRAGONRAGE
  "Une charge brutale et intimidante. Peut apeurer la cible.",  // DRAGONRUSH
  "Projette la cible et la force \205 \210tre remplac\202e. Contre un Pok\202mon sauvage, finit le combat.",  // DRAGONTAIL
  "Un baiser qui draine les PV de la cible pour restaurer ceux du lanceur.",  // DRAININGKISS
  "Un coup qui draine l'\202nergie. Le lanceur r\202cup\212re la moiti\202 des d\202g\203ts inflig\202s.",  // DRAINPUNCH
  "D\202vore le r\210ve d'une cible endormie. R\202cup\212re la moiti\202 des d\202g\203ts inflig\202s.",  // DREAMEATER
  "Un coup de bec tournoyant et per\207ant tr\212s puissant.",  // DRILLPECK
  "Le lanceur tourne comme une perceuse et percute la cible. Taux de critique \202lev\202.",  // DRILLRUN
  "Le lanceur contr\223le une souche en tambourinant. Baisse la Vitesse de la cible.",  // DRUMBEATING
  "Frappe la cible 2 fois de suite avec sa queue ou une autre partie du corps.",  // DUALCHOP
  "Frappe l'ennemi 2 fois avec ses ailes.",  // DUALWINGBEAT
  "Le lanceur tire un puissant rayon d'\202nergie depuis son noyau.",  // DYNAMAXCANNON
  "Un coup de poing de toute sa force. Rend la cible confuse s'il touche.",  // DYNAMICPUNCH
  "Le sol entre en \202ruption sous l'ennemi. Peut baisser sa D\202f. Sp\202.",  // EARTHPOWER
  "Un s\202isme qui frappe tous les Pok\202mon autour du lanceur.",  // EARTHQUAKE
  "Un murmure dont la puissance augmente \205 chaque utilisation par le lanceur ou d'autres.",  // ECHOEDVOICE
  "\220met des ondes \202tranges qui baissent beaucoup l'Atq. Sp\202. de la cible.",  // EERIEIMPULSE
  "Lance un oeuf sur la cible avec une grande force.",  // EGGBOMB
  "Cr\202e un Champ \220lectrifi\202 pendant 5 tours. Les Pok\202mon au sol ne peuvent pas s'endormir.",  // ELECTRICTERRAIN
  "Lance une boule \202lectrique. Plus le lanceur est rapide, plus les d\202g\203ts sont importants.",  // ELECTROBALL
  "Fonce sur l'ennemi en se transformant. Plus puissant si l'attaque est super efficace.",  // ELECTRODRIFT
  "1er tour : accumule de l'\202lectricit\202 et augmente l'Atq. Sp\202. 2e tour : attaque. Sous la pluie, attaque au 1er tour.",  // ELECTROSHOT
  "Capture les ennemis dans une toile \202lectrique. Baisse aussi leur Vitesse.",  // ELECTROWEB
  "De petites flammes qui peuvent br\226ler la cible.",  // EMBER
  "Force la cible \205 r\202p\202ter sa derni\212re capacit\202 pendant 3 tours.",  // ENCORE
  "R\202duit les PV de la cible au niveau de ceux du lanceur.",  // ENDEAVOR
  "Le lanceur r\202siste \205 toute attaque avec au moins 1 PV. Peut \202chouer si utilis\202e de suite.",  // ENDURE
  "Puise la force de la nature et la lib\212re. Peut baisser la D\202f. Sp\202. de la cible.",  // ENERGYBALL
  "Une danse \202trange qui donne \205 la cible le m\210me talent que le lanceur.",  // ENTRAINMENT
  "Une fureur explosive. Moins le lanceur a de PV, moins l'attaque est puissante.",  // ERUPTION
  "Provoque un terrible frisson chez la cible et la g\212le.",  // ESCALOFRIO
  "Tranche avec des ailes charg\202es d'aura. Taux de critique \202lev\202. Augmente la Vitesse.",  // ESPERWING
  "L'attaque la plus puissante d'\220thernatos sous sa forme originelle. Le lanceur ne peut pas agir au tour suivant.",  // ETERNABEAM
  "Attaque avec des pouvoirs psychiques. Sur Champ Psychique, plus puissante et frappe tous les ennemis.",  // EXPANDINGFORCE
  "Le lanceur explose et blesse tous les Pok\202mon autour de lui. Il est aussit\223t mis K.O.",  // EXPLOSION
  "Une \202nergie tr\212s \202trange qui peut apeurer la cible.",  // EXTRASENSORY
  "Une attaque tr\212s rapide qui frappe toujours en premier.",  // EXTREMESPEED
  "Puissance doubl\202e si le lanceur est br\226l\202, paralys\202 ou empoisonn\202.",  // FACADE
  "Le lanceur d\202cha\214ne un vent f\202erique qui fouette la cible.",  // FAIRYWIND
  "Frappe en premier et apeure la cible. Ne marche qu'au premier tour.",  // FAKEOUT
  "De fausses larmes qui baissent beaucoup la D\202f. Sp\202. de la cible.",  // FAKETEARS
  "Le lanceur fait mine de saluer, puis transperce la cible de ses cheveux. N'\202choue jamais.",  // FALSESURRENDER
  "Une attaque retenue qui laisse au moins 1 PV \205 la cible.",  // FALSESWIPE
  "Enveloppe la cible de plumes, ce qui baisse beaucoup son Attaque.",  // FEATHERDANCE
  "Frappe une cible qui utilise Abri ou D\202tection et annule ces capacit\202s.",  // FEINT
  "Trompe la cible pour s'approcher et frappe sans jamais \202chouer.",  // FEINTATTACK
  "Si cette capacit\202 met la cible K.O., l'Attaque du lanceur augmente \202norm\202ment.",  // FELLSTINGER
  "Parfois, les autres t\210tes du lanceur se joignent \205 l'attaque et doublent les d\202g\203ts.",  // FICKLEBEAM
  "Le lanceur attaque envelopp\202 de flammes. Peut augmenter son Atq. Sp\202.",  // FIERYDANCE
  "Le lanceur se sacrifie et inflige des d\202g\203ts \202gaux aux PV qui lui restaient.",  // FINALGAMBIT
  "Une flamme intense qui roussit la cible. Peut la br\226ler.",  // FIREBLAST
  "Mord avec des crocs enflamm\202s. Peut apeurer ou br\226ler la cible.",  // FIREFANG
  "Frappe l'ennemi avec un fouet incandescent et baisse sa D\202fense.",  // FIRELASH
  "Un coup de poing enflamm\202. Peut br\226ler la cible.",  // FIREPUNCH
  "Un anneau de feu pi\212ge la cible pendant 4 \205 5 tours.",  // FIRESPIN
  "Capacit\202 puissante et prioritaire, utilisable seulement au premier tour du lanceur.",  // FIRSTIMPRESSION
  "Attaque avec ses branchies. Puissance doubl\202e si le lanceur agit avant la cible.",  // FISHIOUSREND
  "Fait tomber la cible dans une crevasse. La met K.O. en un coup.",  // FISSURE
  "Une attaque fr\202n\202tique. Moins le lanceur a de PV, plus les d\202g\203ts sont \202lev\202s.",  // FLAIL
  "Frappe la cible d'une gerbe de flammes qui touche aussi les Pok\202mon voisins.",  // FLAMEBURST
  "Frappe la cible avec des flammes et augmente la Vitesse du lanceur.",  // FLAMECHARGE
  "Une grande rafale de feu qui peut br\226ler la cible.",  // FLAMETHROWER
  "Le lanceur s'entoure de feu et charge. Peut br\226ler la cible.",  // FLAMEWHEEL
  "Le lanceur s'embrase et charge la cible, mais subit aussi des d\202g\203ts.",  // FLAREBLITZ
  "Une lumi\212re aveuglante qui baisse la Pr\202cision de la cible.",  // FLASH
  "Concentre toute la lumi\212re de son corps et la lib\212re. Peut baisser la D\202f. Sp\202. de la cible.",  // FLASHCANNON
  "Flatte la cible et la rend confuse, mais augmente aussi son Atq. Sp\202.",  // FLATTER
  "Le lanceur tire un rayon puissant, mais son Atq. Sp\202. baisse beaucoup.",  // FLEURCANNON
  "Lance l'objet tenu sur la cible. Puissance et effet d\202pendent de l'objet.",  // FLING
  "Apr\212s l'attaque, le lanceur c\212de vite sa place \205 un autre Pok\202mon de l'\202quipe.",  // FLIPTURN
  "Lance un bouquet pi\202g\202. N'\202choue jamais et fait toujours un coup critique.",  // FLOWERTRICK
  "S'envole au 1er tour, puis attaque la cible au 2e.",  // FLY
  "Se jette sur l'ennemi. Cette capacit\202 est \205 la fois de type Combat et Vol.",  // FLYINGPRESS
  "Le lanceur se concentre et lib\212re sa puissance. Peut baisser la D\202f. Sp\202. de la cible.",  // FOCUSBLAST
  "Concentre son \202nergie pour augmenter le taux de critique.",  // FOCUSENERGY
  "Se concentre avant de frapper. \220choue si le lanceur est touch\202 avant d'agir.",  // FOCUSPUNCH
  "Frappe la cible d'une onde de choc. Peut la paralyser.",  // FORCEPALM
  "Permet de toucher les Pok\202mon Spectre avec toute capacit\202 et les cibles \202vasives.",  // FORESIGHT
  "Le lanceur utilise l'Attaque de la cible pour calculer les d\202g\203ts.",  // FOULPLAY
  "Refroidit brutalement la cible. Peut la geler. Super efficace contre le type Eau.",  // FREEZEDRY
  "Attaque avec un cristal de brume glac\202e. Annule les changements de stats de tous.",  // FREEZYFROST
  "Souffle un air glacial sur la cible. Fait toujours un coup critique.",  // FROSTBREATH
  "Plus le lanceur est peu attach\202 \205 son Dresseur, plus l'attaque est puissante.",  // FRUSTRATION
  "Frappe la cible de sa corne ou de son bec 2 \205 5 fois de suite.",  // FURYATTACK
  "Une attaque de griffes ou de faux qui gagne en puissance si utilis\202e de suite.",  // FURYCUTTER
  "Griffe rapidement la cible 2 \205 5 fois de suite.",  // FURYSWIPES
  "Une \202norme d\202charge \202lectrique. Plus puissante si elle suit une immense flamme.",  // FUSIONBOLT
  "Une flamme gigantesque. Plus puissante si elle suit une immense d\202charge \202lectrique.",  // FUSIONFLARE
  "Concentre une \202nergie psychique qui frappe la cible 2 tours plus tard.",  // FUTURESIGHT
  "Le lanceur crache ses sucs gastriques sur la cible, ce qui annule son talent.",  // GASTROACID
  "Lance deux engrenages d'acier sur la cible. Frappe 2 fois.",  // GEARGRIND
  "Accumule de l'\202nergie au 1er tour, puis augmente beaucoup Atq. Sp\202., D\202f. Sp\202. et Vitesse au 2e.",  // GEOMANCY
  "Absorbe la moiti\202 des d\202g\203ts inflig\202s pour restaurer les PV du lanceur.",  // GIGADRAIN
  "Le lanceur charge la cible de toutes ses forces. Il doit se reposer au tour suivant.",  // GIGAIMPACT
  "Frappe avec un \202norme marteau. Ne peut pas \210tre utilis\202e 2 fois de suite.",  // GIGATONHAMMER
  "Un souffle glacial qui peut baisser la Vitesse de la cible.",  // GLACIATE
  "Au tour suivant, les attaques ennemies ne ratent pas le lanceur et lui infligent le double de d\202g\203ts.",  // GLAIVERUSH
  "Le lanceur terrifie la cible d'un regard qui la paralyse.",  // GLARE
  "Bombarde la cible de force t\202l\202kin\202tique et met en place Mur Lumi\212re.",  // GLITZYGLOW
  "Fait tr\202bucher la cible avec de l'herbe. Plus elle est lourde, plus les d\202g\203ts sont \202lev\202s.",  // GRASSKNOT
  "Une douce m\202lodie qui endort la cible.",  // GRASSWHISTLE
  "Attaque en glissant sur le sol. Prioritaire sur Champ Herbu.",  // GRASSYGLIDE
  "Cr\202e un Champ Herbu pendant 5 tours. Les Pok\202mon au sol r\202cup\212rent des PV \205 chaque fin de tour.",  // GRASSYTERRAIN
  "Fait tomber une pomme de tr\212s haut sur la cible. Baisse sa D\202fense.",  // GRAVAPPLE
  "Un grognement mignon qui baisse l'Attaque de l'ennemi.",  // GROWL
  "Le corps du lanceur grandit \205 vue d'oeil, ce qui augmente son Attaque et son Atq. Sp\202.",  // GROWTH
  "Si le lanceur est mis K.O. par une attaque, celle-ci perd tous ses PP.",  // GRUDGE
  "Une attaque de pinces qui met l'ennemi K.O. en un coup si elle touche.",  // GUILLOTINE
  "Projette des ordures immondes sur la cible. Peut l'empoisonner.",  // GUNKSHOT
  "Cr\202e une tornade avec ses ailes et la lance sur la cible.",  // GUST
  "Une charge tournoyante. Plus le lanceur est lent, plus les d\202g\203ts sont \202lev\202s.",  // GYROBALL
  "Frappe lourdement avec son poing. Baisse cependant la Vitesse du lanceur.",  // HAMMERARM
  "Le lanceur contracte ses muscles, ce qui augmente sa D\202fense.",  // HARDEN
  "Une brume qui annule les changements de stats de tous les Pok\202mon au combat.",  // HAZE
  "Un puissant coup de t\210te. Peut apeurer la cible.",  // HEADBUTT
  "Un \202norme coup de t\210te. Le lanceur subit aussi un peu de d\202g\203ts.",  // HEADCHARGE
  "Le lanceur charge de toutes ses forces, mais sa D\202fense et sa D\202f. Sp\202. baissent.",  // HEADLONGRUSH
  "Un coup de t\210te de toutes ses forces, au p\202ril de sa vie. Le lanceur subit de lourds d\202g\203ts.",  // HEADSMASH
  "Une cloche apaisante qui soigne les probl\212mes de statut de toute l'\202quipe.",  // HEALBELL
  "Le lanceur est mis K.O., mais son rempla\207ant r\202cup\212re tous ses PV et son statut.",  // HEALINGWISH
  "Le lanceur appelle ses sbires pour le soigner. R\202cup\212re jusqu'\205 la moiti\202 de ses PV max.",  // HEALORDER
  "Une onde curative qui restaure la moiti\202 des PV max de la cible.",  // HEALPULSE
  "Un geste gracieux qui frappe tr\212s fort. Peut apeurer la cible.",  // HEARTSTAMP
  "Plus le lanceur est lourd par rapport \205 la cible, plus les d\202g\203ts sont \202lev\202s.",  // HEATCRASH
  "Un vent br\226lant qui peut br\226ler la cible.",  // HEATWAVE
  "Plus le lanceur est lourd par rapport \205 la cible, plus les d\202g\203ts sont \202lev\202s.",  // HEAVYSLAM
  "Inflige d'\202normes d\202g\203ts \205 une cible qui souffre d'un probl\212me de statut.",  // HEX
  "Une attaque unique dont le type et la puissance d\202pendent du lanceur.",  // HIDDENPOWER
  "Le lanceur frappe violemment la cible de tout son corps.",  // HIGHHORSEPOWER
  "Saute tr\212s haut pour donner un coup de genou. S'il rate, le lanceur se blesse.",  // HIGHJUMPKICK
  "Le lanceur s'aiguise les griffes, ce qui augmente son Attaque et sa Pr\202cision.",  // HONECLAWS
  "Frappe la cible d'un coup de corne.",  // HORNATTACK
  "Une corne tournoyante qui met la cible K.O. en un coup si elle touche.",  // HORNDRILL
  "Un coup qui draine l'\202nergie. Le lanceur r\202cup\212re la moiti\202 des d\202g\203ts inflig\202s.",  // HORNLEECH
  "Un hurlement qui augmente l'Attaque du lanceur.",  // HOWL
  "Un violent tourbillon enveloppe la cible. Peut la rendre confuse.",  // HURRICANE
  "Projette une \202norme masse d'eau sous haute pression.",  // HYDROPUMP
  "Projette un puissant jet d'eau bouillante. Plus puissant au soleil.",  // HYDROSTEAM
  "Un rayon d\202vastateur, mais le lanceur doit se reposer au tour suivant.",  // HYPERBEAM
  "Attaque en faisant tourner la partie pointue de son corps. Ignore les capacit\202s comme Abri.",  // HYPERDRILL
  "Attaque avec des crocs ac\202r\202s. Peut apeurer la cible.",  // HYPERFANG
  "Frappe m\210me si la cible utilise Abri, D\202tection, Garde Large ou Pr\202vention ce tour-ci.",  // HYPERSPACEHOLE
  "Un cri d\202chirant qui blesse la cible.",  // HYPERVOICE
  "Une suggestion hypnotique qui plonge la cible dans un profond sommeil.",  // HYPNOSIS
  "Le lanceur roule sur la cible pendant 5 tours, de plus en plus fort.",  // ICEBALL
  "Un rayon de glace qui peut geler la cible.",  // ICEBEAM
  "Mord avec des crocs glac\202s. Peut apeurer ou geler la cible.",  // ICEFANG
  "Un terrible coup de poing frappe l'ennemi, mais la Vitesse du lanceur baisse.",  // ICEHAMMER
  "Un coup de poing glac\202. Peut geler la cible.",  // ICEPUNCH
  "Cr\202e des blocs de glace et les lance \205 grande vitesse. Frappe toujours en premier.",  // ICESHARD
  "Couvre ses membres de glace pour frapper l'ennemi. D\202truit le champ actif.",  // ICESPINNER
  "Lance de grosses stalactites. Peut apeurer la cible.",  // ICICLECRASH
  "Lance des stalactites ac\202r\202es sur la cible 2 \205 5 fois de suite.",  // ICICLESPEAR
  "Un vent glac\202 qui baisse la Vitesse de la cible.",  // ICYWIND
  "Les ennemis ne peuvent plus utiliser les capacit\202s que conna\214t le lanceur.",  // IMPRISON
  "Des flammes frappent les cibles voisines. Les Baies qu'elles tiennent br\226lent et disparaissent.",  // INCINERATE
  "Une grande rafale de feu qui br\226le la cible.",  // INFERNO
  "Harc\212le l'ennemi pendant 4 \205 5 tours. Il ne peut pas fuir pendant ce temps.",  // INFESTATION
  "Le lanceur plante ses racines et r\202cup\212re des PV \205 chaque tour, mais ne peut plus \210tre remplac\202.",  // INGRAIN
  "Disperse des particules charg\202es qui changent les capacit\202s Normal en type \220lectrik.",  // IONDELUGE
  "Le lanceur durcit son corps comme du fer, ce qui augmente beaucoup sa D\202fense.",  // IRONDEFENSE
  "Frappe avec sa t\210te dure comme le fer. Peut apeurer la cible.",  // IRONHEAD
  "Frappe avec une queue de fer. Peut baisser la D\202fense de la cible.",  // IRONTAIL
  "Frappe avec une massue. Le type d\202pend du masque port\202. Taux de critique \202lev\202.",  // IVYCUDGEL
  "Ni la cible ni le lanceur ne peuvent quitter le combat avant que l'un d'eux soit K.O.",  // JAWLOCK
  "Enveloppe son poing d'un torrent et frappe en priorit\202.",  // JETPUNCH
  "\220met d'innombrables rayons de lumi\212re. Le type d\202pend de la Plaque tenue par le lanceur.",  // JUDGMENT
  "Saute et donne un coup de pied. S'il rate, le lanceur se blesse.",  // JUMPKICK
  "Restaure les PV et soigne les probl\212mes de statut du lanceur et de ses alli\202s.",  // JUNGLEHEALING
  "Une manchette tranchante. Taux de critique \202lev\202.",  // KARATECHOP
  "Tord une cuill\212re pour distraire la cible, ce qui baisse sa Pr\202cision.",  // KINESIS
  "Prot\212ge le lanceur de tout d\202g\203t et baisse l'Attaque de l'ennemi qui le touche.",  // KINGSSHIELD
  "Emp\210che la cible d'utiliser son objet tenu pendant le combat.",  // KNOCKOFF
  "Le lanceur s'incline pour que la cible baisse sa garde, puis attaque. N'\202choue jamais.",  // KOWTOWCLEAVE
  "Rassemble l'\202nergie de la cro\226te terrestre et la concentre pour blesser les ennemis.",  // LANDSWRATH
  "Le lanceur se concentre pour que sa prochaine attaque soit un coup critique.",  // LASERFOCUS
  "Utilisable seulement apr\212s avoir utilis\202 toutes les autres capacit\202s du Pok\202mon.",  // LASTRESORT
  "Venge les alli\202s tomb\202s. Plus il y a d'alli\202s K.O., plus l'attaque est puissante.",  // LASTRESPECTS
  "Un torrent de flammes frappe les Pok\202mon voisins. Peut les br\226ler.",  // LAVAPLUME
  "Attaque l'ennemi en lui lan\207ant des feuilles.",  // LEAFAGE
  "Tranche avec une feuille fine comme une lame. Taux de critique \202lev\202.",  // LEAFBLADE
  "Une temp\210te de feuilles ac\202r\202es. Baisse beaucoup l'Atq. Sp\202. du lanceur.",  // LEAFSTORM
  "Attaque avec des feuilles ac\202r\202es. Peut baisser la Pr\202cision de la cible.",  // LEAFTORNADO
  "Le lanceur r\202cup\212re la moiti\202 des d\202g\203ts inflig\202s \205 la cible.",  // LEECHLIFE
  "Plante des graines qui drainent les PV de la cible \205 chaque tour pour soigner le lanceur.",  // LEECHSEED
  "Un regard intimidant qui baisse la D\202fense de la cible.",  // LEER
  "L\212che la cible avec une longue langue. Peut la paralyser.",  // LICK
  "R\202pand une eau myst\202rieuse qui restaure les PV du lanceur et de ses alli\202s.",  // LIFEDEW
  "Un mur de lumi\212re qui r\202duit les d\202g\203ts des attaques sp\202ciales pendant 5 tours.",  // LIGHTSCREEN
  "Frappe avec la force de l'eau. Peut aussi baisser la D\202fense de la cible.",  // LIQUIDATION
  "Verrouille la cible pour que la prochaine attaque ne rate pas.",  // LOCKON
  "Le lanceur fait une t\210te effrayante et embrasse la cible, qui s'endort.",  // LOVELYKISS
  "Un coup de pied bas. Plus la cible est lourde, plus les d\202g\203ts sont \202lev\202s.",  // LOWKICK
  "Une attaque rapide aux jambes qui baisse la Vitesse de la cible.",  // LOWSWEEP
  "Lance une incantation au ciel qui emp\210che la cible de faire des coups critiques.",  // LUCKYCHANT
  "Projette une \202trange lumi\212re qui affecte l'esprit et baisse beaucoup la D\202f. Sp\202.",  // LUMINACRASH
  "Une pri\212re \205 la lune qui restaure les PV et soigne les probl\212mes de statut de l'\202quipe.",  // LUNARBLESSING
  "Le lanceur est mis K.O., mais son rempla\207ant r\202cup\212re tous ses PV, ses PP et son statut.",  // LUNARDANCE
  "Se jette sur l'ennemi de toutes ses forces et baisse son Attaque.",  // LUNGE
  "Un \202clair de lumi\212re qui peut baisser la D\202f. Sp\202. de la cible.",  // LUSTERPURGE
  "Un coup de poing ultra-rapide qui frappe toujours en premier.",  // MACHPUNCH
  "R\202pand d'\202tranges feuilles qui poursuivent la cible. N'\202choue jamais.",  // MAGICALLEAF
  "La cible est pi\202g\202e dans une temp\210te de feu pendant 2 \205 5 tours.",  // MAGMASTORM
  "Lance des bombes de fer qui collent \205 l'ennemi. N'\202choue jamais.",  // MAGNETBOMB
  "Le lanceur l\202vite gr\203ce \205 un champ magn\202tique pendant 5 tours.",  // MAGNETRISE
  "Une secousse sismique de puissance variable qui frappe les Pok\202mon voisins.",  // MAGNITUDE
  "Lance une pluie de pi\212ces, mais baisse l'Atq. Sp\202. du lanceur. Augmente l'argent gagn\202.",  // MAKEITRAIN
  "Encha\214ne la cible pour saper ses forces. Peut l'empoisonner gravement.",  // MALIGNANTCHAIN
  "Asperge la cible de th\202 et r\202cup\212re la moiti\202 des d\202g\203ts inflig\202s. Peut br\226ler.",  // MATCHAGOTCHA
  "Emp\210che la cible de fuir le combat.",  // MEANLOOK
  "Le lanceur m\202dite, ce qui augmente son Attaque.",  // MEDITATE
  "Absorbe la moiti\202 des d\202g\203ts inflig\202s pour restaurer les PV du lanceur.",  // MEGADRAIN
  "Une charge violente avec des cornes imposantes.",  // MEGAHORN
  "Un coup de pied d'une force extr\210me.",  // MEGAKICK
  "Un coup de poing tr\212s puissant.",  // MEGAPUNCH
  "Le lanceur est mis K.O., mais baisse beaucoup l'Attaque et l'Atq. Sp\202. de la cible.",  // MEMENTO
  "Renvoie \205 la cible la derni\212re attaque subie, avec beaucoup plus de force.",  // METALBURST
  "Griffes d'acier qui peuvent augmenter l'Attaque du lanceur.",  // METALCLAW
  "Un horrible bruit m\202tallique qui baisse beaucoup la D\202f. Sp\202. de la cible.",  // METALSOUND
  "Le lanceur frappe avec son poireau. Il ne peut pas agir au tour suivant.",  // METEORASSAULT
  "1er tour : augmente l'Atq. Sp\202. 2e tour : attaque la cible.",  // METEORBEAM
  "Un coup de poing comme un m\202t\202ore. Peut augmenter l'Attaque du lanceur.",  // METEORMASH
  "Tranche la cible avec la lumi\212re amass\202e sur sa t\210te. Frappe m\210me si la cible se prot\212ge.",  // MIGHTYCLEAVE
  "Restaure la moiti\202 des PV max. Hors combat, peut donner des PV \205 un alli\202.",  // MILKDRINK
  "Fait exploser sa t\210te pour frapper tous les Pok\202mon autour, mais se blesse aussi.",  // MINDBLOWN
  "Le lanceur devine les mouvements de la cible pour que sa prochaine attaque ne rate pas.",  // MINDREADER
  "Le lanceur rapetisse, ce qui augmente beaucoup son Esquive.",  // MINIMIZE
  "Permet de toucher les Pok\202mon T\202n\212bres avec toute capacit\202 et les cibles \202vasives.",  // MIRACLEEYE
  "Riposte \205 une attaque sp\202ciale en infligeant le double des d\202g\203ts subis.",  // MIRRORCOAT
  "Riposte avec la m\210me capacit\202 que la cible a utilis\202e.",  // MIRRORMOVE
  "Lib\212re un \202clat d'\202nergie depuis son corps poli. Peut baisser la Pr\202cision.",  // MIRRORSHOT
  "Une fine brume prot\212ge les stats de l'\202quipe contre les baisses pendant 5 tours.",  // MIST
  "Un nuage de brume qui peut baisser l'Atq. Sp\202. de la cible.",  // MISTBALL
  "Une brume myst\202rieuse. Priorit\202 accrue sur Champ Brumeux.",  // MISTYEXPLOSION
  "Couvre le terrain de brume pendant 5 tours. Les Pok\202mon au sol sont prot\202g\202s des probl\212mes de statut.",  // MISTYTERRAIN
  "Invoque la puissance de la lune pour attaquer. Peut baisser l'Atq. Sp\202. de la cible.",  // MOONBLAST
  "Un myst\202rieux rayon d'\202nergie lunaire. Ignore le talent de la cible.",  // MOONGEISTBEAM
  "Restaure les PV du lanceur. La quantit\202 d\202pend de la m\202t\202o.",  // MOONLIGHT
  "Restaure les PV du lanceur. La quantit\202 d\202pend de la m\202t\202o.",  // MORNINGSUN
  "Une attaque tournoyante qui empoisonne la cible et annule les pi\212ges et Vampigraine.",  // MORTALSPIN
  "Lance des blocs de glace gros comme des icebergs. Peut apeurer la cible.",  // MOUNTAINGALE
  "Lance une boule de boue compacte. Peut baisser la Pr\202cision de la cible.",  // MUDBOMB
  "Projette de l'eau boueuse. Peut baisser la Pr\202cision de la cible.",  // MUDDYWATER
  "Lance de la boue sur la cible et baisse sa Vitesse.",  // MUDSHOT
  "Lance de la boue au visage de la cible pour baisser sa Pr\202cision.",  // MUDSLAP
  "Le lanceur se couvre de boue, ce qui affaiblit les capacit\202s \220lectrik tant qu'il est au combat.",  // MUDSPORT
  "Frappe avec une puissante \202nergie. Le type d\202pend de la ROM tenue.",  // MULTIATTACK
  "Souffle une flamme br\226lante sur la cible, ce qui baisse son Atq. Sp\202.",  // MYSTICALFIRE
  "Lib\212re un pouvoir myst\202rieux qui augmente aussi l'Atq. Sp\202. du lanceur.",  // MYSTICALPOWER
  "Le lanceur a de mauvaises pens\202es, ce qui augmente \202norm\202ment son Atq. Sp\202.",  // NASTYPLOT
  "La Baie tenue donne sa force \205 l'attaque. Le type et la puissance d\202pendent de la Baie.",  // NATURALGIFT
  "Frappe la cible avec la fureur de la nature et divise ses PV par 2.",  // NATURESMADNESS
  "Frappe avec des bras \202pineux. Peut apeurer la cible.",  // NEEDLEARM
  "Une onde t\202n\202breuse frappe la cible. Peut baisser sa Pr\202cision.",  // NIGHTDAZE
  "Cr\202e un mirage. La cible perd autant de PV que le niveau du lanceur.",  // NIGHTSHADE
  "Tranche la cible \205 la premi\212re occasion. Taux de critique \202lev\202.",  // NIGHTSLASH
  "Un cri de guerre intimidant qui baisse l'Attaque et l'Atq. Sp\202. de l'ennemi.",  // NOBLEROAR
  "Le lanceur augmente toutes ses stats, mais ne peut plus fuir ni \210tre remplac\202.",  // NORETREAT
  "Frotte ses joues \202lectriques contre la cible, ce qui la paralyse.",  // NUZZLE
  "Absorbe l'\202nergie de la cible et r\202cup\212re 3/4 des d\202g\203ts inflig\202s.",  // OBLIVIONWING
  "Bloque les attaques. Une attaque au contact baisse beaucoup la D\202fense de l'agresseur. \220choue si utilis\202e trop souvent.",  // OBSTRUCT
  "Crache de l'encre au visage de la cible. Peut baisser sa Pr\202cision.",  // OCTAZOOKA
  "Retient la cible pour l'emp\210cher de fuir et baisse sa D\202fense et sa D\202f. Sp\202. \205 chaque tour.",  // OCTOLOCK
  "Permet de toucher les Pok\202mon Spectre avec toute capacit\202 et les cibles \202vasives.",  // ODORSLEUTH
  "Un vent horrifiant. Peut augmenter toutes les stats du lanceur d'un coup.",  // OMINOUSWIND
  "Si un Nigirigon est dans sa bouche, augmente une stat selon la forme de celui-ci.",  // ORDERUP
  "Frappe la cible avec d'innombrables rayons de lumi\212re bleut\202e.",  // ORIGINPULSE
  "Attaque pendant 2 \205 3 tours, puis le lanceur devient confus.",  // OUTRAGE
  "Le lanceur gratte son corps et cr\202e d'\202normes vibrations pour attaquer la cible.",  // OVERDRIVE
  "Une attaque \205 pleine puissance qui baisse beaucoup l'Atq. Sp\202. du lanceur.",  // OVERHEAT
  "Additionne les PV du lanceur et de la cible, puis les partage \205 parts \202gales.",  // PAINSPLIT
  "Blesse les Pok\202mon autour du lanceur, qui r\202cup\212re la moiti\202 des d\202g\203ts inflig\202s.",  // PARABOLICCHARGE
  "Baisse l'Attaque et l'Atq. Sp\202. de l'ennemi, puis le lanceur est remplac\202.",  // PARTINGSHOT
  "Puissance doubl\202e si la cible a agi avant le lanceur.",  // PAYBACK
  "Lance des pi\212ces sur la cible. On les r\202cup\212re \205 la fin du combat.",  // PAYDAY
  "Frappe la cible avec une corne ou un bec pointu.",  // PECK
  "D\202cha\214ne une violente temp\210te de p\202tales qui blesse tous les Pok\202mon autour.",  // PETALBLIZZARD
  "Lance des p\202tales pendant 2 \205 3 tours, puis le lanceur devient confus.",  // PETALDANCE
  "Dispara\214t au 1er tour et attaque au 2e. Frappe m\210me si la cible se prot\212ge.",  // PHANTOMFORCE
  "Compare l'Attaque et l'Atq. Sp\202. du lanceur et utilise la plus \202lev\202e.",  // PHOTONGEYSER
  "Tire de fines aiguilles qui frappent 2 \205 5 fois.",  // PINMISSILE
  "Frappe avec ses poings. Change les capacit\202s Normal en type \220lectrik.",  // PLASMAFISTS
  "Le lanceur devient ami avec l'ennemi, qui perd l'envie de se battre. Baisse son Attaque.",  // PLAYNICE
  "Fait des c\203lins brutaux \205 la cible. Peut baisser son Attaque.",  // PLAYROUGH
  "Picore la cible. Si elle tient une Baie, le lanceur la mange et profite de son effet.",  // PLUCK
  "Des crocs toxiques qui peuvent gravement empoisonner la cible.",  // POISONFANG
  "Projette un nuage de gaz toxique sur la cible et l'empoisonne.",  // POISONGAS
  "Pique l'ennemi avec un bras ou un tentacule venimeux. Peut l'empoisonner.",  // POISONJAB
  "Une poudre toxique qui empoisonne la cible.",  // POISONPOWDER
  "Un dard toxique qui peut empoisonner la cible.",  // POISONSTING
  "Un coup de queue qui peut empoisonner. Taux de critique \202lev\202.",  // POISONTAIL
  "Lance une boule explosive sur l'ennemi. Si elle touche un alli\202, elle restaure ses PV.",  // POLLENPUFF
  "Attaque avec l'objet que tient la cible. \220choue si elle n'en tient pas.",  // POLTERGEIST
  "Une attaque group\202e qui frappe la cible 1 \205 10 fois de suite.",  // POPULATIONBOMB
  "Bondit sur la cible et baisse sa Vitesse.",  // POUNCE
  "Frappe la cible avec les pattes ou la queue.",  // POUND
  "Projette de la neige poudreuse. Peut geler la cible.",  // POWDERSNOW
  "Un rayon de lumi\212re qui scintille comme s'il \202tait fait de milliers de joyaux.",  // POWERGEM
  "Plus les stats du lanceur ont augment\202, plus l'attaque est puissante.",  // POWERTRIP
  "Ses poings durcissent \205 chaque coup. S'il touche, l'Attaque du lanceur augmente.",  // POWERUPPUNCH
  "Le lanceur agite violemment ses lianes ou tentacules pour fouetter la cible.",  // POWERWHIP
  "Manifeste la puissance de la terre sous forme de lames ac\202r\202es pour frapper la cible.",  // PRECIPICEBLADES
  "Un cadeau pi\202g\202 qui explose. Restaure parfois les PV de la cible.",  // PRESENT
  "Tire un rayon puissant \205 travers un prisme. Le lanceur ne peut pas agir au tour suivant.",  // PRISMATICLASER
  "Bloque les attaques, mais peut \202chouer si utilis\202e plusieurs fois de suite.",  // PROTECT
  "Un rayon \202trange qui peut rendre la cible confuse.",  // PSYBEAM
  "Lac\212re la cible d'une lame \202th\202r\202e. Plus puissante sur Champ \220lectrifi\202.",  // PSYBLADE
  "Une puissante attaque t\202l\202kin\202tique. Peut baisser la D\202f. Sp\202. de la cible.",  // PSYCHIC
  "Attaque psychique qui brise aussi les barri\212res comme Mur Lumi\212re et Protection.",  // PSYCHICFANGS
  "Pendant 5 tours, les capacit\202s Psy sont renforc\202es et les capacit\202s prioritaires \202chouent.",  // PSYCHICTERRAIN
  "Une attaque \205 pleine puissance qui baisse beaucoup l'Atq. Sp\202. du lanceur.",  // PSYCHOBOOST
  "Lac\212re la cible avec des lames d'\202nergie psychique. Taux de critique \202lev\202.",  // PSYCHOCUT
  "Le lanceur s'hypnotise et copie les changements de stats de la cible.",  // PSYCHUP
  "Attaque envelopp\202 d'\202nergie psychique, ce qui augmente aussi sa D\202fense.",  // PSYSHIELDBASH
  "Cr\202e une onde psychique qui inflige de lourds d\202g\203ts physiques \205 la cible.",  // PSYSHOCK
  "Cr\202e une onde psychique \202trange qui inflige des d\202g\203ts physiques \205 la cible.",  // PSYSTRIKE
  "Une onde d'\202nergie d'intensit\202 variable.",  // PSYWAVE
  "Plus la cible a augment\202 ses stats, plus l'attaque est puissante.",  // PUNISHMENT
  "Soigne le probl\212me de statut de la cible et restaure en \202change les PV du lanceur.",  // PURIFY
  "Inflige le double de d\202g\203ts \205 une cible qui est en train d'\210tre remplac\202e.",  // PURSUIT
  "Enflamme un petit caillou pour cr\202er une boule de feu. Peut br\226ler la cible.",  // PYROBALL
  "Une attaque rapide qui frappe toujours en premier.",  // QUICKATTACK
  "Prot\212ge l'\202quipe des capacit\202s prioritaires. Peut \202chouer si utilis\202e de suite.",  // QUICKGUARD
  "Une danse mystique qui augmente l'Atq. Sp\202., la D\202f. Sp\202. et la Vitesse.",  // QUIVERDANCE
  "Tant qu'elle est active, l'Attaque du lanceur augmente \205 chaque coup re\207u.",  // RAGE
  "Change sa col\212re en \202nergie. Plus le lanceur a re\207u de coups, plus l'attaque est puissante.",  // RAGEFIST
  "Le type de cette capacit\202 d\202pend de la forme du lanceur. Brise les barri\212res.",  // RAGINGBULL
  "D\202cha\214ne de violentes flammes pendant 2 \205 3 tours, puis le lanceur devient confus.",  // RAGINGFURY
  "Provoque une forte pluie qui renforce les capacit\202s Eau pendant 5 tours.",  // RAINDANCE
  "Une attaque tournoyante qui augmente la Vitesse et retire les pi\212ges, Vampigraine et Picots.",  // RAPIDSPIN
  "Tranche avec des feuilles ac\202r\202es. Taux de critique \202lev\202.",  // RAZORLEAF
  "Tranche la cible avec un coquillage ac\202r\202. Peut baisser sa D\202fense.",  // RAZORSHELL
  "Attaque avec un vent tranchant. Taux de critique \202lev\202.",  // RAZORWIND
  "Restaure jusqu'\205 la moiti\202 des PV max du lanceur.",  // RECOVER
  "Recycle un objet tenu \205 usage unique d\202j\205 utilis\202 en combat pour le r\202utiliser.",  // RECYCLE
  "Un mur de lumi\212re qui r\202duit les d\202g\203ts des attaques physiques pendant 5 tours.",  // REFLECT
  "Le lanceur se repose pour soigner la paralysie, le poison ou une br\226lure.",  // REFRESH
  "Un chant antique qui \202meut les ennemis autour. Peut les endormir.",  // RELICSONG
  "Restaure les PV et soigne les probl\212mes de statut. Le lanceur dort pendant 2 tours.",  // REST
  "Puissance accrue si un alli\202 a \202t\202 mis K.O. au tour pr\202c\202dent.",  // RETALIATE
  "Plus le lanceur est attach\202 \205 son Dresseur, plus l'attaque est puissante.",  // RETURN
  "Une danse \202nergique du m\210me type que le 1er type du lanceur. S'il l'a perdu, prend le 2e.",  // REVELATIONDANCE
  "Inflige le double de d\202g\203ts si le lanceur a \202t\202 bless\202 ce tour-ci.",  // REVENGE
  "Une attaque d\202sesp\202r\202e. Moins le lanceur a de PV, plus les d\202g\203ts sont \202lev\202s.",  // REVERSAL
  "Puissance doubl\202e si la cible est sur un Champ \220lectrifi\202.",  // RISINGVOLTAGE
  "Effraie la cible, qui est remplac\202e. Contre un Pok\202mon sauvage, finit le combat.",  // ROAR
  "Une \202nergie qui distord le temps. Le lanceur doit se reposer au tour suivant.",  // ROAROFTIME
  "Lance des rochers sur la cible 2 \205 5 fois de suite.",  // ROCKBLAST
  "Une charge puissante. Peut rendre la cible confuse.",  // ROCKCLIMB
  "Le lanceur polit son corps pour r\202duire la r\202sistance. Augmente beaucoup sa Vitesse.",  // ROCKPOLISH
  "Lance de gros rochers. Peut apeurer la cible.",  // ROCKSLIDE
  "Un coup de poing qui brise les rochers. Peut baisser la D\202fense de la cible.",  // ROCKSMASH
  "Lance un petit rocher sur la cible.",  // ROCKTHROW
  "Lance des rochers qui bloquent la cible et baissent sa Vitesse.",  // ROCKTOMB
  "Lance un \202norme rocher sur la cible. Le lanceur doit se reposer au tour suivant.",  // ROCKWRECKER
  "Un coup de pied circulaire rapide. Peut apeurer la cible.",  // ROLLINGKICK
  "Le lanceur se roule en boule et roule sur le terrain comme une roue.",  // ROLLOUT
  "Le lanceur se pose au sol pour se reposer. R\202cup\212re jusqu'\205 la moiti\202 de ses PV max.",  // ROOST
  "Un chant qui attaque la cible. Plus il y a de Pok\202mon qui l'utilisent, plus il est puissant.",  // ROUND
  "Provoque une catastrophe d\202vastatrice qui divise par 2 les PV de la cible.",  // RUINATION
  "Une flamme mystique tr\212s intense. Peut br\226ler la cible.",  // SACREDFIRE
  "Attaque avec une \202p\202e en ignorant les changements de stats de la cible.",  // SACREDSWORD
  "Cr\202e un champ protecteur qui emp\210che les probl\212mes de statut pendant 5 tours.",  // SAFEGUARD
  "La cible perd des PV \205 chaque tour. Plus efficace contre les types Acier et Eau.",  // SALTCURE
  "Lance du sable au visage de la cible et baisse sa Pr\202cision.",  // SANDATTACK
  "Enveloppe la cible de sable br\226lant. Peut la br\226ler.",  // SANDSEARSTORM
  "Une temp\210te de sable de 5 tours qui blesse tous les Pok\202mon sauf les types Roche, Sol et Acier.",  // SANDSTORM
  "Pi\212ge la cible dans un tourbillon de sable pendant 4 \205 5 tours.",  // SANDTOMB
  "R\202pand des graines pour attaquer la cible et lui plante Vampigraine.",  // SAPPYSEED
  "Projette de l'eau bouillante sur la cible. Peut la br\226ler.",  // SCALD
  "Tire des \202cailles 2 \205 5 fois. Augmente la Vitesse du lanceur mais baisse sa D\202fense.",  // SCALESHOT
  "Effraie la cible d'une grimace, ce qui baisse beaucoup sa Vitesse.",  // SCARYFACE
  "Lance du sable br\226lant sur la cible. Peut la br\226ler.",  // SCORCHINGSANDS
  "Griffe la cible avec des griffes ac\202r\202es.",  // SCRATCH
  "Un cri strident qui baisse beaucoup la D\202fense de la cible.",  // SCREECH
  "Un torrent de flammes frappe tous les Pok\202mon autour. Peut les br\226ler.",  // SEARINGSHOT
  "Une attaque au pouvoir secret dont l'effet varie selon le terrain du combat.",  // SECRETPOWER
  "Transperce la cible d'un sabre et la blesse avec un pouvoir mystique.",  // SECRETSWORD
  "Fait pleuvoir sur la cible une salve de graines explosives.",  // SEEDBOMB
  "Lib\212re une onde de choc de son corps. Peut baisser la D\202f. Sp\202. de la cible.",  // SEEDFLARE
  "Projette la cible gr\203ce \205 la gravit\202. Inflige autant de d\202g\203ts que le niveau du lanceur.",  // SEISMICTOSS
  "Le lanceur explose et blesse tous les Pok\202mon autour de lui. Il est aussit\223t mis K.O.",  // SELFDESTRUCT
  "Lance une boule t\202n\202breuse. Peut baisser la D\202f. Sp\202. de la cible.",  // SHADOWBALL
  "Frappe l'ennemi avec un os poss\202d\202 par un esprit. Peut baisser sa D\202fense.",  // SHADOWBONE
  "Lac\212re avec une griffe d'ombre ac\202r\202e. Taux de critique \202lev\202.",  // SHADOWCLAW
  "Dispara\214t au 1er tour et frappe au 2e, m\210me si la cible se prot\212ge.",  // SHADOWFORCE
  "Un coup de poing surgi des ombres qui n'\202choue jamais.",  // SHADOWPUNCH
  "\220tend son ombre et frappe la cible par derri\212re. Frappe toujours en premier.",  // SHADOWSNEAK
  "Le lanceur aiguise son profil, ce qui augmente son Attaque.",  // SHARPEN
  "Un froid polaire qui met la cible K.O. en un coup si elle touche.",  // SHEERCOLD
  "Brise sa coquille : baisse D\202fense et D\202f. Sp\202., mais augmente beaucoup Attaque, Atq. Sp\202. et Vitesse.",  // SHELLSMASH
  "Si le lanceur subit une attaque physique, sa carapace pi\202g\202e explose et blesse l'agresseur.",  // SHELLTRAP
  "Une attaque \202lectrique tr\212s rapide qui n'\202choue jamais.",  // SHOCKWAVE
  "Restaure la moiti\202 des PV max du lanceur. Restaure plus pendant une temp\210te de sable.",  // SHOREUP
  "Un rayon de lumi\212re sinistre. Peut rendre la cible confuse.",  // SIGNALBEAM
  "Un pi\212ge soyeux qui prot\212ge le lanceur. Baisse la Vitesse de l'ennemi qui le touche.",  // SILKTRAP
  "Un vent charg\202 de poudre d'\202cailles. Peut augmenter toutes les stats du lanceur.",  // SILVERWIND
  "Une berceuse qui plonge la cible dans un profond sommeil.",  // SING
  "Le lanceur s'embrase et charge la cible. La br\226le toujours.",  // SIZZLYSLIDE
  "Apprend pour toujours la derni\212re capacit\202 de la cible. Ne sert qu'une fois.",  // SKETCH
  "Attaque la cible par derri\212re en douce et baisse son Atq. Sp\202.",  // SKITTERSMACK
  "1er tour : rentre la t\210te et augmente sa D\202fense. 2e tour : donne un coup de t\210te.",  // SKULLBASH
  "Une attaque en 2 tours. Taux de critique \202lev\202. Peut apeurer la cible.",  // SKYATTACK
  "Un uppercut ascendant tr\212s puissant.",  // SKYUPPERCUT
  "Le lanceur paresse et restaure la moiti\202 de ses PV max.",  // SLACKOFF
  "Frappe la cible avec ses membres.",  // SLAM
  "Attaque avec des lames ou des pinces. Taux de critique \202lev\202.",  // SLASH
  "R\202pand une poudre qui endort la cible.",  // SLEEPPOWDER
  "Lance des d\202tritus sur la cible. Peut l'empoisonner.",  // SLUDGE
  "Lance des d\202tritus sur la cible. Peut l'empoisonner.",  // SLUDGEBOMB
  "Une vague toxique qui blesse les Pok\202mon autour. Peut les empoisonner.",  // SLUDGEWAVE
  "Lance une pierre ou un projectile. Fait tomber au sol une cible qui vole.",  // SMACKDOWN
  "Transperce l'ennemi de ses bois ac\202r\202s. N'\202choue jamais.",  // SMARTSTRIKE
  "Inflige le double de d\202g\203ts \205 une cible paralys\202e, mais soigne sa paralysie.",  // SMELLINGSALTS
  "Attaque avec des gaz toxiques. Peut empoisonner la cible.",  // SMOG
  "Un nuage de fum\202e ou d'encre qui baisse la Pr\202cision de la cible.",  // SMOKESCREEN
  "Pi\212ge la cible dans une m\203choire d'acier pendant 4 \205 5 tours.",  // SNAPTRAP
  "Pousse un cri insupportable qui baisse l'Atq. Sp\202. des cibles.",  // SNARL
  "Attaque la cible en ignorant les talents et capacit\202s qui d\202tournent les attaques.",  // SNIPESHOT
  "Un ronflement bruyant, utilisable seulement endormi. Peut apeurer la cible.",  // SNORE
  "Fait tomber la neige pendant 5 tours, ce qui augmente la D\202fense des Pok\202mon Glace.",  // SNOWSCAPE
  "Un torrent d'eau qui change la cible en Pok\202mon de type Eau.",  // SOAK
  "Restaure la moiti\202 des PV max du lanceur. Hors combat, donne des PV \205 un alli\202.",  // SOFTBOILED
  "Absorbe la lumi\212re au 1er tour, attaque au 2e.",  // SOLARBEAM
  "Concentre l'\202nergie en forme de lame au 1er tour, puis attaque au 2e.",  // SOLARBLADE
  "Une onde de choc qui inflige toujours 20 PV de d\202g\203ts.",  // SONICBOOM
  "D\202chire la cible et l'espace autour d'elle. Taux de critique \202lev\202.",  // SPACIALREND
  "Une charge \202lectrique. Peut paralyser la cible.",  // SPARK
  "Lib\212re des bulles en chantant. Soigne les br\226lures des Pok\202mon touch\202s.",  // SPARKLINGARIA
  "Enveloppe la cible dans un tourbillon. Soigne les probl\212mes de statut de l'\202quipe.",  // SPARKLYSWIRL
  "Se cache dans l'ombre de la cible, vole ses hausses de stats, puis attaque.",  // SPECTRALTHIEF
  "Enserre la cible dans des fils collants pour l'emp\210cher de fuir.",  // SPIDERWEB
  "Tire de fines aiguilles qui frappent 2 \205 5 fois.",  // SPIKECANNON
  "R\202pand des picots dans le camp ennemi. Ils blessent les ennemis qui entrent au combat.",  // SPIKES
  "Bloque l'attaque ennemie et blesse l'agresseur s'il l'a touch\202.",  // SPIKYSHIELD
  "Le lanceur tourne violemment sur lui-m\210me. Baisse beaucoup sa Vitesse.",  // SPINOUT
  "Frappe avec un tel \202lan qu'elle mine le moral de la cible et baisse son Atq. Sp\202.",  // SPIRITBREAK
  "Attaque l'ennemi en clouant son ombre au sol, ce qui l'emp\210che de fuir.",  // SPIRITSHACKLE
  "Le lanceur lib\212re sa rancune et retire 4 PP \205 la derni\212re capacit\202 de la cible.",  // SPITE
  "Lib\212re d'un coup l'\202nergie accumul\202e avec Stockage.",  // SPITUP
  "N'a aucun effet. Le lanceur ne fait que barboter.",  // SPLASH
  "R\202pand des spores qui endorment la cible.",  // SPORE
  "Une temp\210te d'amour et de haine enveloppe la cible. Peut baisser son Attaque.",  // SPRINGTIDESTORM
  "Un pi\212ge de rochers flottants qui blesse les ennemis qui entrent au combat.",  // STEALTHROCK
  "Enveloppe l'ennemi d'une vapeur br\226lante. Peut le br\226ler.",  // STEAMERUPTION
  "Le lanceur se roule en boule et \202crase la cible. Peut l'apeurer.",  // STEAMROLLER
  "Tire un rayon de m\202tal puis\202 dans son corps. Le lanceur se blesse aussi.",  // STEELBEAM
  "Attaque et d\202truit le champ actif. \220choue s'il n'y a pas de champ.",  // STEELROLLER
  "Des ailes d'acier frappent la cible. Peut augmenter la D\202fense du lanceur.",  // STEELWING
  "Tend une toile collante dans le camp ennemi qui baisse la Vitesse des Pok\202mon qui entrent.",  // STICKYWEB
  "Accumule de l'\202nergie et augmente D\202fense et D\202f. Sp\202. Utilisable jusqu'\205 3 fois.",  // STOCKPILE
  "Un violent pi\202tinement qui peut apeurer la cible.",  // STOMP
  "Attaque avec frustration. Puissance doubl\202e si la derni\212re capacit\202 du lanceur a \202chou\202.",  // STOMPINGTANTRUM
  "Frappe avec une hache de pierre. Des \202clats de roche restent autour de la cible.",  // STONEAXE
  "Plante des pierres ac\202r\202es dans la cible. Taux de critique \202lev\202.",  // STONEEDGE
  "Plus les stats du lanceur ont augment\202, plus les d\202g\203ts sont \202lev\202s.",  // STOREDPOWER
  "Un coup fulgurant qui fait toujours un coup critique.",  // STORMTHROW
  "Attaque l'ennemi en \202mettant de la vapeur. Peut rendre la cible confuse.",  // STRANGESTEAM
  "Un puissant coup de poing.",  // STRENGTH
  "Baisse l'Attaque de la cible et restaure autant de PV que la valeur de son Attaque.",  // STRENGTHSAP
  "Projette de la soie sur la cible et baisse sa Vitesse.",  // STRINGSHOT
  "Le lanceur riposte et baisse aussi l'Atq. Sp\202. des ennemis.",  // STRUGGLEBUG
  "R\202pand une poudre qui paralyse la cible.",  // STUNSPORE
  "Plaque la cible au sol. Blesse aussi le lanceur.",  // SUBMISSION
  "Permet d'attaquer en premier. \220choue si la cible ne pr\202pare pas d'attaque.",  // SUCKERPUNCH
  "Intensifie le soleil pendant 5 tours, ce qui renforce les capacit\202s Feu.",  // SUNNYDAY
  "Frappe la cible avec la puissance d'un m\202t\202ore.",  // SUNSTEELSTRIKE
  "\220lectrifie son corps et se jette sur la cible. S'il rate, le lanceur se blesse.",  // SUPERCELLSLAM
  "Des crocs ac\202r\202s qui divisent par 2 les PV de la cible.",  // SUPERFANG
  "Une attaque tr\212s puissante, mais qui baisse l'Attaque et la D\202fense du lanceur.",  // SUPERPOWER
  "D'\202tranges ondes sonores qui peuvent rendre la cible confuse.",  // SUPERSONIC
  "Inonde le terrain avec une vague g\202ante.",  // SURF
  "Rend la cible confuse, mais augmente aussi beaucoup son Attaque.",  // SWAGGER
  "Absorbe l'\202nergie accumul\202e avec Stockage pour restaurer des PV.",  // SWALLOW
  "Un baiser si doux qu'il rend la cible confuse.",  // SWEETKISS
  "Un doux parfum qui baisse l'Esquive de l'ennemi. Attire aussi les Pok\202mon sauvages.",  // SWEETSCENT
  "Tire des rayons en forme d'\202toile qui n'\202chouent jamais.",  // SWIFT
  "\220change les objets tenus avec la cible, si vite que l'oeil ne peut pas le voir.",  // SWITCHEROO
  "Une danse fr\202n\202tique qui augmente beaucoup l'Attaque.",  // SWORDSDANCE
  "Une onde \202trange qui blesse les Pok\202mon voisins du m\210me type que le lanceur.",  // SYNCHRONOISE
  "Restaure les PV du lanceur. La quantit\202 d\202pend de la m\202t\202o.",  // SYNTHESIS
  "Englue la cible de nectar visqueux, ce qui baisse sa Vitesse pendant 3 tours.",  // SYRUPBOMB
  "Une salve de lames de particules qui frappe 2 fois de suite. N'\202choue jamais.",  // TACHYONCUTTER
  "Charge la cible de tout son corps.",  // TACKLE
  "Se concentre sur une lumi\212re vive, ce qui augmente \202norm\202ment son Atq. Sp\202.",  // TAILGLOW
  "Frappe avec sa queue ou une autre partie du corps 2 \205 5 fois de suite.",  // TAILSLAP
  "Remue la queue pour baisser la D\202fense de l'ennemi.",  // TAILWHIP
  "Un vent fort qui double la Vitesse des Pok\202mon de l'\202quipe pendant 4 tours.",  // TAILWIND
  "Une charge t\202m\202raire qui blesse aussi le lanceur.",  // TAKEDOWN
  "Le lanceur soigne ses probl\212mes de statut et augmente son Atq. Sp\202. et sa D\202f. Sp\202.",  // TAKEHEART
  "Couvre la cible de goudron, ce qui baisse sa Vitesse et la rend vuln\202rable au type Feu.",  // TARSHOT
  "Provoque la cible, qui ne peut utiliser que des capacit\202s offensives pendant 3 tours.",  // TAUNT
  "Un regard larmoyant qui \223te l'envie de se battre. Baisse l'Attaque et l'Atq. Sp\202. de la cible.",  // TEARFULLOOK
  "Tire un grand laser sur la cible. Le type d\202pend du Module tenu par le lanceur.",  // TECHNOBLAST
  "Une danse chancelante qui rend confus tous les Pok\202mon autour du lanceur.",  // TEETERDANCE
  "Utilis\202e par Terapagos sous sa Forme Stellaire, blesse tous les ennemis.",  // TERASTARSTORM
  "Utilise l'\202nergie du champ actif, qui d\202termine le type et la puissance.",  // TERRAINPULSE
  "Attaque et vole l'objet de la cible. \220choue si le lanceur tient d\202j\205 un objet.",  // THIEF
  "Touche m\210me les Pok\202mon en vol et les fait tomber au sol.",  // THOUSANDARROWS
  "Des ondes sismiques frappent tous les ennemis par le sol. Ceux touch\202s ne peuvent plus fuir.",  // THOUSANDWAVES
  "Attaque pendant 2 \205 3 tours, puis le lanceur devient confus.",  // THRASH
  "Une douleur si intense que la cible ne peut plus utiliser de capacit\202s sonores pendant 2 tours.",  // THROATCHOP
  "Un puissant \202clair qui blesse la cible. Peut la paralyser.",  // THUNDER
  "Une puissante attaque \202lectrique. Peut paralyser la cible.",  // THUNDERBOLT
  "Enferme la cible dans une cage \202lectrique pendant 4 \205 5 tours.",  // THUNDERCAGE
  "Un \202clair frappe la cible en priorit\202. \220choue si la cible n'attaque pas.",  // THUNDERCLAP
  "Mord avec des crocs \202lectrifi\202s. Peut apeurer ou paralyser la cible.",  // THUNDERFANG
  "Un coup de poing \202lectrique. Peut paralyser la cible.",  // THUNDERPUNCH
  "Une attaque \202lectrique qui peut paralyser la cible.",  // THUNDERSHOCK
  "Une faible d\202charge \202lectrique qui paralyse la cible si elle touche.",  // THUNDERWAVE
  "Chatouille la cible pour baisser son Attaque et sa D\202fense.",  // TICKLE
  "Augmente l'Attaque et la Vitesse. Retire les effets comme Picots, Pi\212ge de Roc ou Clonage.",  // TIDYUP
  "Crache des flammes br\226lantes comme s'il chantait. Augmente l'Atq. Sp\202.",  // TORCHSONG
  "Tourmente la cible, qui ne peut plus utiliser la m\210me capacit\202 2 fois de suite.",  // TORMENT
  "Empoisonne gravement la cible. Les d\202g\203ts du poison augmentent \205 chaque tour.",  // TOXIC
  "R\202pand des pics toxiques dans le camp ennemi. Ils empoisonnent les Pok\202mon qui entrent.",  // TOXICSPIKES
  "Attaque avec des fils toxiques qui baissent la Vitesse de l'ennemi et l'empoisonnent.",  // TOXICTHREAD
  "Le lanceur se transforme en copie de la cible, avec les m\210mes capacit\202s.",  // TRANSFORM
  "Une triple attaque qui peut paralyser, br\226ler ou geler la cible.",  // TRIATTACK
  "Prend la cible au d\202pourvu et \202change les objets tenus avec elle.",  // TRICK
  "Un triple plongeon parfaitement synchronis\202 qui frappe la cible 3 fois de suite.",  // TRIPLEDIVE
  "Frappe jusqu'\205 3 fois de suite, de plus en plus fort.",  // TRIPLEHIT
  "Donne jusqu'\205 3 coups de pied de suite, de plus en plus forts.",  // TRIPLEKICK
  "Un coup de pied \205 la force tropicale qui baisse l'Attaque de l'ennemi.",  // TROPKICK
  "Moins il reste de PP \205 cette capacit\202, plus elle est puissante.",  // TRUMPCARD
  "\220met de ses yeux deux myst\202rieux rayons de lumi\212re qui frappent 2 fois de suite.",  // TWINBEAM
  "Pique l'ennemi 2 fois avec des dards. Peut l'empoisonner.",  // TWINEEDLE
  "Cr\202e une violente tornade qui d\202chiquette la cible. Peut l'apeurer.",  // TWISTER
  "Attaque avec vacarme pendant 3 tours. Emp\210che tout le monde de dormir.",  // UPROAR
  "Apr\212s l'attaque, le lanceur revient en vitesse et c\212de sa place \205 un autre Pok\202mon.",  // UTURN
  "Fait tourner ses poings et lance une onde de vide. Frappe toujours en premier.",  // VACUUMWAVE
  "Frappe avec une flamme en V. Baisse la D\202fense, la D\202f. Sp\202. et la Vitesse du lanceur.",  // VCREATE
  "Baisse l'Attaque, l'Atq. Sp\202. et la Vitesse, mais seulement d'un Pok\202mon empoisonn\202.",  // VENOMDRENCH
  "Asperge la cible d'un liquide toxique. D\202g\203ts doubl\202s si elle est d\202j\205 empoisonn\202e.",  // VENOSHOCK
  "Fouette la cible avec de fines lianes.",  // VINEWHIP
  "Serre la cible avec de puissantes pinces.",  // VISEGRIP
  "Le lanceur agit en dernier, mais n'\202choue jamais.",  // VITALTHROW
  "Apr\212s l'attaque, le lanceur revient en vitesse et c\212de sa place \205 un autre Pok\202mon.",  // VOLTSWITCH
  "Une charge \202lectrique tr\212s risqu\202e qui blesse aussi le lanceur.",  // VOLTTACKLE
  "Inflige de gros d\202g\203ts \205 une cible endormie, mais la r\202veille aussi.",  // WAKEUPSLAP
  "Une charge puissante qui peut apeurer la cible. Sert aussi \205 remonter les cascades.",  // WATERFALL
  "Projette de l'eau avec une grande puissance.",  // WATERGUN
  "Un puissant jet d'eau. Peut rendre la cible confuse.",  // WATERPULSE
  "Frappe 2 \205 5 fois avec des \202toiles d'eau. Frappe toujours en premier.",  // WATERSHURIKEN
  "Le lanceur se mouille, ce qui affaiblit les capacit\202s Feu tant qu'il est au combat.",  // WATERSPORT
  "Un jet d'eau. Moins le lanceur a de PV, moins l'attaque est puissante.",  // WATERSPOUT
  "Le lanceur s'enveloppe d'eau et charge la cible, mais se blesse gravement aussi.",  // WAVECRASH
  "Le type et la puissance de l'attaque d\202pendent de la m\202t\202o.",  // WEATHERBALL
  "Un tourbillon d'eau pi\212ge la cible pendant 4 \205 5 tours.",  // WHIRLPOOL
  "Souffle l'ennemi, qui est remplac\202. Contre un Pok\202mon sauvage, finit le combat.",  // WHIRLWIND
  "Un coup d\202vastateur qui exige une ma\214trise totale des T\202n\212bres. Toujours critique.",  // WICKEDBLOW
  "Invoque un orage qui frappe la cible de vent et d'\202clairs. Peut la paralyser.",  // WILDBOLTSTORM
  "Une charge \202lectrique tr\212s puissante qui blesse aussi un peu le lanceur.",  // WILDCHARGE
  "Une sinistre flamme violette qui br\226le la cible.",  // WILLOWISP
  "Frappe la cible avec de grandes ailes.",  // WINGATTACK
  "Le lanceur se prot\212ge dans sa carapace et augmente sa D\202fense.",  // WITHDRAW
  "Percute l'ennemi de son corps robuste. Le lanceur subit aussi de lourds d\202g\203ts.",  // WOODHAMMER
  "Augmente l'Attaque et l'Atq. Sp\202. du lanceur.",  // WORKUP
  "Plante une graine qui tourmente la cible et lui donne le talent Insomnia.",  // WORRYSEED
  "Ligote et serre la cible avec des lianes ou son corps pendant 4 \205 5 tours.",  // WRAP
  "Essore la cible avec force. Plus elle a de PV, plus l'attaque est puissante.",  // WRINGOUT
  "Croise ses faux ou ses griffes pour frapper la cible comme des ciseaux.",  // XSCISSOR
  "Un grand b\203illement qui endort la cible au tour suivant.",  // YAWN
  "Tire une d\202charge \202lectrique qui blesse et paralyse.",  // ZAPCANNON
  "Concentre son \202nergie psychique dans sa t\210te pour frapper. Peut apeurer la cible.",  // ZENHEADBUTT
  "Se jette sur la cible et lib\212re une puissante d\202charge. Peut l'apeurer.",  // ZINGZAP
  "Emp\210che la cible d'utiliser son objet tenu. Son Dresseur ne peut pas non plus lui donner d'objets.",  // EMBARGO
  "Emp\210che la cible d'utiliser capacit\202s, talents et objets qui rendent des PV pendant 5 tours.",  // HEALBLOCK
  "Tout Pok\202mon qui entend ce chant est mis K.O. dans 3 tours s'il n'est pas remplac\202.",  // PERISHSONG
  "Sacrifie une partie de ses PV pour cr\202er un clone qui sert de leurre.",  // SUBSTITUTE
  "Cr\202e un espace \202trange o\227 les Pok\202mon lents agissent en premier pendant 5 tours.",  // TRICKROOM
  "Un voeu qui restaure la moiti\202 des PV max au tour suivant.",  // WISH
};

static const char *const MOVE_DESC_DE[MOVE_COUNT] = {
  "Wird eingesetzt, wenn keine Attacken mehr \201brig sind. Schadet auch dem Anwender.",  // STRUGGLE
  "Absorbiert die H\204lfte des verursachten Schadens als KP.",  // ABSORB
  "Der Anwender rammt das Ziel mit hohem Tempo. Diese Attacke greift immer zuerst an.",  // ACCELEROCK
  "Verspr\201ht \204tzende S\204ure. Senkt eventuell die Spezial-Verteidigung des Ziels.",  // ACID
  "Der Anwender verfl\201ssigt seine Zellstruktur und erh\224ht seine Verteidigung stark.",  // ACIDARMOR
  "Greift mit \204tzender Fl\201ssigkeit an und senkt die Sp.-Vert. des Ziels stark.",  // ACIDSPRAY
  "Ein flinker Angriff. Tr\204gt der Anwender kein Item, richtet er enormen Schaden an.",  // ACROBATICS
  "Druck auf wichtige K\224rperpunkte erh\224ht einen zuf\204lligen Statuswert stark.",  // ACUPRESSURE
  "Der Anwender greift blitzschnell an. Diese Attacke trifft immer.",  // AERIALACE
  "Feuert einen Luftstrahl ab. Hohe Volltrefferquote.",  // AEROBLAST
  "Der Anwender entspannt seinen K\224rper und erh\224ht seine Initiative stark.",  // AGILITY
  "Ein peitschender, schneidender Wind. Hohe Volltrefferquote.",  // AIRCUTTER
  "Greift mit einer Luftklinge an, die den Himmel zerschneidet. Ziel schreckt eventuell zur\201ck.",  // AIRSLASH
  "Der Anwender vergisst seine Sorgen und erh\224ht seine Spezial-Verteidigung stark.",  // AMNESIA
  "Wirft einen Anker auf das Ziel. Es wird festgehalten und kann nicht fliehen.",  // ANCHORSHOT
  "Ein urzeitlicher Angriff. Erh\224ht eventuell alle Statuswerte des Anwenders.",  // ANCIENTPOWER
  "Greift mit \204tzendem Saft eines sauren Apfels an und senkt die Sp.-Vert. des Ziels.",  // APPLEACID
  "Schneidet das Ziel wie mit einer Klinge aus Wasser unter Druck. Hohe Volltrefferquote.",  // AQUACUTTER
  "Ein schneller Angriff, bei dem der Anwender zuerst zuschl\204gt.",  // AQUAJET
  "Ein Schleier aus Wasser umh\201llt den Anwender. Er f\201llt jede Runde einige KP auf.",  // AQUARING
  "Ein eleganter, flie\341ender Tanz, der Schaden zuf\201gt. Erh\224ht die Initiative des Anwenders.",  // AQUASTEP
  "Der Anwender schl\204gt mit dem Schweif zu wie eine tobende Welle im Sturm.",  // AQUATAIL
  "Schleudert Teile seiner R\201stung als brennende Geschosse. Senkt Vert. und Sp.-Vert. des Anwenders.",  // ARMORCANNON
  "Direkte St\224\341e, die das Ziel 2- bis 5-mal hintereinander treffen.",  // ARMTHRUST
  "Ein sanfter Duft heilt die Statusprobleme des ganzen Teams.",  // AROMATHERAPY
  "Ein mysteri\224ser Duft erh\224ht die Spezial-Verteidigung eines Pok\202mon im Team.",  // AROMATICMIST
  "Hat das Ziel in dieser Runde schon Schaden erlitten, verdoppelt sich die St\204rke.",  // ASSURANCE
  "Ein so verbl\201ffender Angriff, dass das Ziel eventuell zur\201ckschreckt.",  // ASTONISH
  "Bei\341t mit Kiefern zu, die gegen K\204fer-Pok\202mon besonders effektiv sind.",  // ATRAPAMOSCAS
  "Der Anwender ruft seine Untergebenen zum Angriff. Hohe Volltrefferquote.",  // ATTACKORDER
  "Ist das Ziel vom anderen Geschlecht, verliebt es sich und greift seltener an.",  // ATTRACT
  "Setzt eine Welle aus Aura-Kraft frei. Trifft immer.",  // AURASPHERE
  "Greift an und erh\224ht die Initiative. Der Typ h\204ngt von der Form des Anwenders ab.",  // AURAWHEEL
  "Ein bunter Strahl. Senkt eventuell den Angriff des Ziels.",  // AURORABEAM
  "Der Anwender wirft Ballast ab, wird leichter und erh\224ht seine Initiative stark.",  // AUTOTOMIZE
  "Doppelter Schaden, wenn das Ziel den Anwender in dieser Runde getroffen hat.",  // AVALANCHE
  "Ein Tritt aus der Luft mit der Ferse. Bei Fehlschlag nimmt der Anwender Schaden. Kann verwirren.",  // AXEKICK
  "Ein treuer Blick senkt den Angriff des Ziels. Diese Attacke greift immer zuerst an.",  // BABYDOLLEYES
  "Der Anwender benimmt sich daneben, greift an und errichtet einen Reflektor.",  // BADDYBAD
  "Sch\201tzt vor Angriffen und vergiftet Angreifer, die den Anwender ber\201hren.",  // BANEFULBUNKER
  "Feuert giftige Stacheln, die vergiften k\224nnen. Ist das Ziel vergiftet, doppelte St\204rke.",  // BARBBARRAGE
  "Wirft 2- bis 5-mal hintereinander Kugeln auf das Ziel.",  // BARRAGE
  "Errichtet eine Barriere und erh\224ht die Verteidigung stark.",  // BARRIER
  "Tauscht mit einem Team-Pok\202mon und gibt Statusver\204nderungen an es weiter.",  // BATONPASS
  "Erhitzt erst den Schnabel, dann greift er an. Wer ihn dabei physisch angreift, wird verbrannt.",  // BEAKBLAST
  "Das ganze Team greift an. Je mehr Pok\202mon, desto mehr Treffer.",  // BEATUP
  "Schadet dem Ziel mit einem R\201lpser. Der Anwender muss daf\201r eine Beere gegessen haben.",  // BELCH
  "Opfert die H\204lfte der max. KP, um den Angriff zu maximieren.",  // BELLYDRUM
  "H\204lt 2 Runden lang Angriffen stand und schl\204gt dann mit doppeltem Schaden zur\201ck.",  // BIDE
  "Umwickelt und quetscht das Ziel 4 bis 5 Runden lang.",  // BIND
  "Ein kr\204ftiger Biss. Ziel schreckt eventuell zur\201ck.",  // BITE
  "Ein Stich voller Groll. Der Anwender f\201llt KP in H\224he der H\204lfte des Schadens auf.",  // BITTERBLADE
  "Greift das Ziel mit eisigem Groll an und senkt seinen Angriff.",  // BITTERMALICE
  "Ein Tritt mit hoher Volltrefferquote. Kann das Ziel verbrennen.",  // BLAZEKICK
  "Ein eiskalter Wind, der erschaudern l\204sst. Senkt eventuell die Initiative des Ziels.",  // BLEAKWINDSTORM
  "Ein Eissturm, der das Ziel einfrieren kann.",  // BLIZZARD
  "Versperrt dem Ziel den Weg, sodass es nicht fliehen kann.",  // BLOCK
  "H\201llt das Ziel in eine sch\224ne, aber heftige blaue Flamme. Kann verbrennen.",  // BLUEFLARE
  "Rammt das Ziel. Je h\224her die Verteidigung des Anwenders, desto mehr Schaden.",  // BODYPRESS
  "Der Anwender wirft sich mit vollem Gewicht auf das Ziel. Kann paralysieren.",  // BODYSLAM
  "Greift mit elektrisiertem Schnabel an. Handelt der Anwender vor dem Ziel, doppelte St\204rke.",  // BOLTBEAK
  "Der Anwender h\201llt sich in Elektrizit\204t und rammt das Ziel. Kann paralysieren.",  // BOLTSTRIKE
  "Schl\204gt mit einem Knochen zu. Ziel schreckt eventuell zur\201ck.",  // BONECLUB
  "Wirft einen Knochen wie einen Bumerang, der 2-mal trifft.",  // BONEMERANG
  "Schl\204gt das Ziel 2- bis 5-mal mit einem Knochen.",  // BONERUSH
  "Greift alle Pok\202mon in der N\204he mit einer gewaltigen Schallwelle an.",  // BOOMBURST
  "Springt in Runde 1 hoch und trifft in Runde 2. Kann paralysieren.",  // BOUNCE
  "Beschie\341t das Ziel mit Wasserblasen und absorbiert die H\204lfte des Schadens.",  // BOUNCYBUBBLE
  "Sticht das Ziel mit einem spitzen Zweig.",  // BRANCHPOKE
  "Legt die Fl\201gel an und st\201rzt sich auf das Ziel. Der Anwender erleidet R\201cksto\341schaden.",  // BRAVEBIRD
  "Schl\204gt heftig mit dem Schweif zu und senkt den Angriff des Ziels.",  // BREAKINGSWIPE
  "Ein starker Angriff, der auch Barrieren wie Lichtschild und Reflektor zerst\224rt.",  // BRICKBREAK
  "Hat das Ziel nur noch die H\204lfte seiner KP oder weniger, doppelter Schaden.",  // BRINE
  "Der Anwender schwingt seinen K\224rper und schadet allen Gegnern.",  // BRUTALSWING
  "Beschie\341t den Gegner mit Blasen. Senkt eventuell die Initiative.",  // BUBBLE
  "Ein Schwall von Blasen. Senkt eventuell die Initiative.",  // BUBBLEBEAM
  "Pickt das Ziel. Tr\204gt es eine Beere, isst der Anwender sie und erh\204lt ihren Effekt.",  // BUGBITE
  "Vibrierende Fl\201gel erzeugen eine Schallwelle. Senkt eventuell die Spezial-Verteidigung des Ziels.",  // BUGBUZZ
  "St\204hlt den K\224rper und erh\224ht Angriff und Verteidigung.",  // BULKUP
  "Ein Erdsto\341, der benachbarte Pok\202mon trifft und ihre Initiative senkt.",  // BULLDOZE
  "Ein harter Schlag, schnell wie eine Kugel. Diese Attacke greift immer zuerst an.",  // BULLETPUNCH
  "Feuert 2- bis 5-mal hintereinander Samen ab.",  // BULLETSEED
  "Sch\201tzt sich mit seinem Fell vor Angriffen. Wer es ber\201hrt, wird eventuell verbrannt.",  // BURNINGBULWARK
  "Greift mit dem letzten Rest seiner Flammen an, verliert aber den Typ Feuer.",  // BURNUP
  "Greift mit einer elektrischen Entladung an. Paralysiert das Ziel immer.",  // BUZZYBUZZ
  "Konzentriert den Geist und erh\224ht Spezial-Angriff und Spezial-Verteidigung.",  // CALMMIND
  "Ist das Ziel vom anderen Geschlecht, wird es bet\224rt und sein Spezial-Angriff sinkt stark.",  // CAPTIVATE
  "L\204dt Energie f\201r die n\204chste Elektro-Attacke auf. Erh\224ht die Spezial-Verteidigung.",  // CHARGE
  "Feuert einen Elektrostrahl. Erh\224ht eventuell den Spezial-Angriff des Anwenders.",  // CHARGEBEAM
  "Umgarnt das Ziel und senkt seinen Angriff stark.",  // CHARM
  "Greift mit einer Schallwelle aus gelernten W\224rtern an. Kann das Ziel verwirren.",  // CHATTER
  "Bespritzt das Ziel mit eiskaltem, entmutigendem Wasser. Senkt seinen Angriff.",  // CHILLINGWATER
  "Ein so kalter Witz, dass es schneit. Dann tauscht der Anwender mit einem Team-Pok\202mon.",  // CHILLYRECEPTION
  "Trifft die Schwachstellen des Ziels und ignoriert dessen Statusver\204nderungen.",  // CHIPAWAY
  "Feuert geb\201ndeltes Chlorophyll als Strahl. Der Anwender nimmt dabei auch Schaden.",  // CHLOROBLAST
  "Wirft das Ziel weg und zwingt es zum Tausch. Gegen wilde Pok\202mon endet der Kampf.",  // CIRCLETHROW
  "Klemmt das Ziel 4 bis 5 Runden lang fest ein.",  // CLAMP
  "Reibt die Schuppen zu lautem L\204rm aneinander. Senkt die Verteidigung des Anwenders.",  // CLANGINGSCALES
  "Der Anwender erh\224ht alle Statuswerte und opfert daf\201r KP.",  // CLANGOROUSSOUL
  "Greift mit seltsamem Nebel an, der alle Statusver\204nderungen des Ziels aufhebt.",  // CLEARSMOG
  "K\204mpft ungesch\201tzt nah am Ziel. Senkt Verteidigung und Spezial-Verteidigung des Anwenders.",  // CLOSECOMBAT
  "Der Anwender konzentriert sich und erh\224ht Angriff, Verteidigung und Genauigkeit.",  // COIL
  "Kracht beim Verwandeln in den Boden. Ist der Angriff sehr effektiv, steigt die St\204rke.",  // COLLISIONCOURSE
  "Schl\204gt 2- bis 5-mal hintereinander zu.",  // COMETPUNCH
  "Gibt den zuletzt erlittenen Angriff mit viel gr\224\341erer Wucht zur\201ck.",  // COMEUPPANCE
  "Lenkt den Gegner mit einem Geheimnis ab und senkt seinen Spezial-Angriff.",  // CONFIDE
  "Ein unheimlicher Strahl, der das Ziel verwirrt.",  // CONFUSERAY
  "Ein schwacher telekinetischer Angriff. Kann das Ziel verwirren.",  // CONFUSION
  "Umschlingt das Ziel. Senkt eventuell seine Initiative.",  // CONSTRICT
  "Ahmt die zuletzt eingesetzte Attacke nach. Schl\204gt fehl, wenn noch keine eingesetzt wurde.",  // COPYCAT
  "Greift an. Hat das Ziel schon eine Attacke eingesetzt, verliert es seine F\204higkeit.",  // COREENFORCER
  "Mystische Energie erh\224ht Verteidigung und Spezial-Verteidigung des Anwenders.",  // COSMICPOWER
  "H\201llt sich in sch\201tzende Watte und erh\224ht die Verteidigung drastisch.",  // COTTONGUARD
  "Heftet Sporen an das Ziel und senkt seine Initiative stark.",  // COTTONSPORE
  "Kontert physische Angriffe mit doppeltem Schaden.",  // COUNTER
  "N\204hert sich dem Ziel lieb, greift dann an und stiehlt eventuell sein Item.",  // COVET
  "Schl\204gt mit gro\341en Scheren kr\204ftig zu. Hohe Volltrefferquote.",  // CRABHAMMER
  "Ein Doppelhieb mit hoher Volltrefferquote.",  // CROSSCHOP
  "Ein Hieb, der vergiften kann. Hohe Volltrefferquote.",  // CROSSPOISON
  "Zermalmt das Ziel mit scharfen Z\204hnen. Senkt eventuell seine Verteidigung.",  // CRUNCH
  "Zerfetzt das Ziel mit scharfen Klauen. Senkt eventuell seine Verteidigung.",  // CRUSHCLAW
  "Je mehr KP das Ziel noch hat, desto st\204rker ist der Angriff.",  // CRUSHGRIP
  "Wirkt anders, je nachdem, ob der Anwender vom Typ Geist ist oder nicht.",  // CURSE
  "Schneidet mit Klauen, Sicheln usw. Kann auch kleine B\204ume f\204llen.",  // CUT
  "Dreht sich und trifft das Ziel. Ignoriert dessen Statusver\204nderungen.",  // DARKESTLARIAT
  "Setzt eine schreckliche Aura voll finsterer Gedanken frei. Ziel schreckt eventuell zur\201ck.",  // DARKPULSE
  "Zieht das Ziel in eine Welt der Finsternis, wo es einschl\204ft.",  // DARKVOID
  "Schadet den Gegnern mit einem starken Licht.",  // DAZZLINGGLEAM
  "Der Anwender ruft seine Freunde als lebenden Schild. Erh\224ht Vert. und Sp.-Vert.",  // DEFENDORDER
  "Rollt sich ein, um seine Schwachstellen zu verbergen. Erh\224ht die Verteidigung.",  // DEFENSECURL
  "Ein starker Wind beseitigt Reflektor und Lichtschild des Ziels. Senkt eventuell seinen Ausweichwert.",  // DEFOG
  "Gleitet auf Skiern \201ber den Schnee und erh\224ht die Initiative des Anwenders.",  // DESLIZAMIENTO
  "Wird der Anwender besiegt, wird auch der Gegner besiegt.",  // DESTINYBOND
  "Wehrt Angriffe ab, kann aber bei wiederholtem Einsatz fehlschlagen.",  // DETECT
  "Ein verheerender Sturm aus Diamanten. Erh\224ht eventuell die Verteidigung des Anwenders.",  // DIAMONDSTORM
  "Gr\204bt sich in Runde 1 ein und greift in Runde 2 an. Dient auch zum Verlassen bestimmter Orte.",  // DIG
  "Greift die Schwachstelle des Ziels mit t\224dlichen Klauen an. Kann vergiften, paralysieren oder einschl\204fern.",  // DIRECLAW
  "Blockiert die zuletzt eingesetzte Attacke des Ziels f\201r 4 Runden.",  // DISABLE
  "Bet\224rt die Gegner mit seiner Stimme und f\201gt ihnen seelischen Schaden zu. Trifft immer.",  // DISARMINGVOICE
  "Eine grelle Elektrowelle trifft alle anderen Pok\202mon im Kampf. Kann paralysieren.",  // DISCHARGE
  "Taucht in Runde 1 ab und greift in Runde 2 an.",  // DIVE
  "Rhythmische Schl\204ge, die das Ziel verwirren k\224nnen.",  // DIZZYPUNCH
  "B\201ndelt einen Lichtstrahl und greift 2 Runden sp\204ter an.",  // DOOMDESIRE
  "Ein riskanter Angriff, der auch dem Anwender schadet.",  // DOUBLEEDGE
  "Schl\204gt das Ziel 2-mal hintereinander mit dem Schweif oder anderen K\224rperteilen.",  // DOUBLEHIT
  "Schl\204gt 2-mal hintereinander mit den F\204usten zu. Ziel schreckt eventuell zur\201ck.",  // DOUBLEIRONBASH
  "Ein Doppeltritt, der 2-mal trifft.",  // DOUBLEKICK
  "Setzt seine gesamte Elektrizit\204t zum Angriff frei. Verliert den Typ Elektro.",  // DOUBLESHOCK
  "Ohrfeigt das Ziel 2- bis 5-mal hintereinander.",  // DOUBLESLAP
  "Erzeugt Kopien von sich selbst und erh\224ht den Ausweichwert.",  // DOUBLETEAM
  "L\204sst gro\341e Kometen vom Himmel fallen. Senkt den Spezial-Angriff des Anwenders stark.",  // DRACOMETEOR
  "St\201rzt mit hohem Tempo vom Himmel. Senkt Vert. und Sp.-Vert. des Anwenders.",  // DRAGONASCENT
  "Ein m\204chtiger Atemsto\341, der Schaden zuf\201gt. Kann paralysieren.",  // DRAGONBREATH
  "Kratzt das Ziel mit scharfen Klauen.",  // DRAGONCLAW
  "Ein mystischer Tanz, der Angriff und Initiative erh\224ht.",  // DRAGONDANCE
  "Greift 2-mal mit Grolldra an. Bei 2 Gegnern trifft er jeden 1-mal.",  // DRAGONDARTS
  "Greift mit seiner Lebenskraft an. Je weniger KP der Anwender hat, desto geringer der Schaden.",  // DRAGONENERGY
  "Schleudert seinen K\224rper wie einen Hammer auf das Ziel.",  // DRAGONHAMMER
  "Rei\341t das Maul weit auf und greift mit einer Schockwelle an.",  // DRAGONPULSE
  "Eine Schockwelle voller Wut. Zieht immer 40 KP ab.",  // DRAGONRAGE
  "Ein brutaler, einsch\201chternder Angriff. Ziel schreckt eventuell zur\201ck.",  // DRAGONRUSH
  "Schleudert das Ziel weg und zwingt es zum Tausch. Gegen wilde Pok\202mon endet der Kampf.",  // DRAGONTAIL
  "Saugt dem Ziel mit einem Kuss KP ab und f\201llt die eigenen auf.",  // DRAININGKISS
  "Ein Schlag, der Energie absaugt. Der Anwender f\201llt KP in H\224he der H\204lfte des Schadens auf.",  // DRAINPUNCH
  "Frisst die Tr\204ume eines schlafenden Ziels. F\201llt KP in H\224he der H\204lfte des Schadens auf.",  // DREAMEATER
  "Ein starker, bohrender Schnabelhieb.",  // DRILLPECK
  "Rammt das Ziel, sich drehend wie ein Bohrer. Hohe Volltrefferquote.",  // DRILLRUN
  "Steuert einen Baumstumpf per Trommelschlag. Senkt die Initiative des Ziels.",  // DRUMBEATING
  "Schl\204gt 2-mal hintereinander mit dem Schweif oder anderen K\224rperteilen zu.",  // DUALCHOP
  "Schl\204gt den Gegner 2-mal mit den Fl\201geln.",  // DUALWINGBEAT
  "Feuert einen gewaltigen Energiestrahl aus seinem Kern.",  // DYNAMAXCANNON
  "Ein Schlag mit voller Konzentration. Verwirrt das Ziel, wenn er trifft.",  // DYNAMICPUNCH
  "L\204sst den Boden unter dem Ziel ausbrechen. Senkt eventuell seine Spezial-Verteidigung.",  // EARTHPOWER
  "Ein Erdbeben, das alle Pok\202mon in der N\204he trifft.",  // EARTHQUAKE
  "Ein Fl\201stern, das st\204rker wird, je \224fter der Anwender und andere Pok\202mon es einsetzen.",  // ECHOEDVOICE
  "Sendet seltsame Wellen aus, die den Spezial-Angriff des Ziels stark senken.",  // EERIEIMPULSE
  "Schleudert ein gro\341es Ei mit gro\341er Wucht auf das Ziel.",  // EGGBOMB
  "Erzeugt 5 Runden lang ein Elektrofeld. Pok\202mon am Boden k\224nnen nicht einschlafen.",  // ELECTRICTERRAIN
  "Wirft eine Elektrokugel. Je h\224her die Initiative des Anwenders, desto mehr Schaden.",  // ELECTROBALL
  "St\201rzt sich beim Verwandeln auf das Ziel. Ist der Angriff sehr effektiv, steigt die St\204rke.",  // ELECTRODRIFT
  "Runde 1: sammelt Strom und erh\224ht den Spezial-Angriff. Runde 2: Angriff. Bei Regen greift er sofort an.",  // ELECTROSHOT
  "F\204ngt die Ziele in einem Elektronetz und senkt ihre Initiative.",  // ELECTROWEB
  "Greift mit kleinen Flammen an. Kann verbrennen.",  // EMBER
  "Das Ziel muss 3 Runden lang seine letzte Attacke wiederholen.",  // ENCORE
  "Senkt die KP des Ziels auf den Wert der KP des Anwenders.",  // ENDEAVOR
  "\232bersteht jeden Angriff mit mindestens 1 KP. Kann bei wiederholtem Einsatz fehlschlagen.",  // ENDURE
  "Sammelt die Kr\204fte der Natur und greift an. Senkt eventuell die Sp.-Vert. des Ziels.",  // ENERGYBALL
  "Ein seltsamer Tanz, der dem Ziel die gleiche F\204higkeit wie dem Anwender gibt.",  // ENTRAINMENT
  "Explosive Wut. Je weniger KP der Anwender hat, desto geringer der Schaden.",  // ERUPTION
  "L\204sst das Ziel schrecklich fr\224steln und friert es ein.",  // ESCALOFRIO
  "Schneidet mit von Aura erf\201llten Fl\201geln. Hohe Volltrefferquote. Erh\224ht die Initiative.",  // ESPERWING
  "Endynalos' st\204rkster Angriff in seiner Urform. Der Anwender muss danach 1 Runde aussetzen.",  // ETERNABEAM
  "Greift mit Psycho-Kr\204ften an. Auf Psychofeld steigt die St\204rke und alle Gegner werden getroffen.",  // EXPANDINGFORCE
  "Der Anwender explodiert und schadet allen Pok\202mon um sich herum. Er wird sofort besiegt.",  // EXPLOSION
  "Eine sehr seltsame Energie. Ziel schreckt eventuell zur\201ck.",  // EXTRASENSORY
  "Ein blitzschneller Angriff, der immer zuerst ausgef\201hrt wird.",  // EXTREMESPEED
  "Doppelte St\204rke, wenn der Anwender verbrannt, paralysiert oder vergiftet ist.",  // FACADE
  "Entfesselt einen Feenwind, der auf das Ziel einschl\204gt.",  // FAIRYWIND
  "Greift zuerst an und l\204sst das Ziel zur\201ckschrecken. Wirkt nur in der ersten Runde.",  // FAKEOUT
  "Krokodilstr\204nen senken die Spezial-Verteidigung des Ziels stark.",  // FAKETEARS
  "T\204uscht eine Verbeugung vor und sticht mit wirrem Haar zu. Trifft immer.",  // FALSESURRENDER
  "Ein gez\201gelter Angriff, der dem Ziel mindestens 1 KP l\204sst.",  // FALSESWIPE
  "H\201llt das Ziel in Daunen und senkt seinen Angriff stark.",  // FEATHERDANCE
  "Trifft Ziele, die Schutzschild oder Scanner einsetzen, und hebt diese Attacken auf.",  // FEINT
  "Lockt das Ziel in eine Falle und schl\204gt dann zu. Trifft immer.",  // FEINTATTACK
  "Besiegt der Anwender das Ziel mit dieser Attacke, steigt sein Angriff drastisch.",  // FELLSTINGER
  "Manchmal greifen die anderen K\224pfe mit an und richten doppelten Schaden an.",  // FICKLEBEAM
  "Greift in Flammen geh\201llt an. Erh\224ht eventuell den Sp.-Ang. des Anwenders.",  // FIERYDANCE
  "Der Anwender opfert sich und f\201gt Schaden in H\224he seiner verbliebenen KP zu.",  // FINALGAMBIT
  "Eine heftige Flamme, die das Ziel versengt. Kann verbrennen.",  // FIREBLAST
  "Bei\341t mit flammenden Z\204hnen zu. Ziel schreckt eventuell zur\201ck oder wird verbrannt.",  // FIREFANG
  "Schl\204gt mit einer gl\201henden Peitsche zu und senkt die Verteidigung des Ziels.",  // FIRELASH
  "Ein feuriger Schlag. Kann verbrennen.",  // FIREPUNCH
  "Ein Feuerring h\204lt das Ziel 4 bis 5 Runden lang fest.",  // FIRESPIN
  "Eine starke Erstschlag-Attacke, die nur in der ersten Runde des Anwenders wirkt.",  // FIRSTIMPRESSION
  "Greift mit den Kiemen an. Handelt der Anwender vor dem Ziel, doppelte St\204rke.",  // FISHIOUSREND
  "Wirft das Ziel in eine Erdspalte. Es wird sofort besiegt.",  // FISSURE
  "Ein wilder Angriff. Je weniger KP der Anwender hat, desto mehr Schaden.",  // FLAIL
  "Trifft das Ziel mit einer Stichflamme, die auch benachbarte Pok\202mon trifft.",  // FLAMEBURST
  "Greift mit Flammen an und erh\224ht die Initiative des Anwenders.",  // FLAMECHARGE
  "Ein starker Feuersto\341. Kann verbrennen.",  // FLAMETHROWER
  "Greift in Feuer geh\201llt an. Kann verbrennen.",  // FLAMEWHEEL
  "H\201llt sich in Flammen und st\201rmt los. Der Anwender erleidet R\201cksto\341schaden.",  // FLAREBLITZ
  "Ein grelles Licht, das die Genauigkeit des Ziels senkt.",  // FLASH
  "B\201ndelt alles Licht im K\224rper und setzt es frei. Senkt eventuell die Spezial-Verteidigung des Ziels.",  // FLASHCANNON
  "Schmeichelt dem Ziel und verwirrt es, erh\224ht aber auch seinen Spezial-Angriff.",  // FLATTER
  "Feuert einen starken Strahl ab. Senkt den Spezial-Angriff des Anwenders stark.",  // FLEURCANNON
  "Schleudert das getragene Item. St\204rke und Effekt h\204ngen vom Item ab.",  // FLING
  "Nach dem Angriff tauscht der Anwender rasch mit einem Team-Pok\202mon.",  // FLIPTURN
  "Wirft einen Trickstrau\341. Trifft immer und ist immer ein Volltreffer.",  // FLOWERTRICK
  "Fliegt in Runde 1 hoch und greift in Runde 2 an.",  // FLY
  "St\201rzt sich auf das Ziel. Diese Attacke ist zugleich vom Typ Kampf und Flug.",  // FLYINGPRESS
  "Sch\204rft die Konzentration und setzt seine Kraft frei. Senkt eventuell die Spezial-Verteidigung des Ziels.",  // FOCUSBLAST
  "Sammelt Energie und erh\224ht die Volltrefferquote.",  // FOCUSENERGY
  "Konzentriert sich vor dem Schlag. Wird der Anwender vorher getroffen, schl\204gt er fehl.",  // FOCUSPUNCH
  "Trifft das Ziel mit einer Schockwelle. Kann paralysieren.",  // FORCEPALM
  "Erm\224glicht es, Geist-Pok\202mon mit jeder Attacke zu treffen, und trifft ausweichende Gegner.",  // FORESIGHT
  "Der Anwender nutzt den Angriff des Ziels zur Schadensberechnung.",  // FOULPLAY
  "K\201hlt das Ziel schlagartig ab und kann es einfrieren. Sehr effektiv gegen Wasser-Pok\202mon.",  // FREEZEDRY
  "Greift mit einem Kristall aus gefrorenem Nebel an. Hebt alle Statusver\204nderungen aller Pok\202mon auf.",  // FREEZYFROST
  "Bl\204st eisige Luft auf das Ziel. Immer ein Volltreffer.",  // FROSTBREATH
  "Je geringer die Freundschaft zum Trainer, desto st\204rker ist diese Attacke.",  // FRUSTRATION
  "Spie\341t das Ziel 2- bis 5-mal hintereinander auf.",  // FURYATTACK
  "Ein Angriff mit Klauen oder Sicheln, der bei wiederholtem Einsatz st\204rker wird.",  // FURYCUTTER
  "Kratzt das Ziel rasch 2- bis 5-mal hintereinander.",  // FURYSWIPES
  "Ein riesiger Stromschlag. Wird st\204rker durch den Einfluss einer gewaltigen Flamme.",  // FUSIONBOLT
  "Eine gewaltige Flamme. Wird st\204rker durch den Einfluss gro\341er elektrischer Energie.",  // FUSIONFLARE
  "B\201ndelt Psycho-Energie, die das Ziel 2 Runden sp\204ter trifft.",  // FUTURESIGHT
  "Bespuckt das Ziel mit Magens\204ure und hebt seine F\204higkeit auf.",  // GASTROACID
  "Wirft 2 Stahlzahnr\204der auf das Ziel. Trifft 2-mal.",  // GEARGRIND
  "Sammelt in Runde 1 Energie und erh\224ht in Runde 2 Spezial-Angriff, Spezial-Verteidigung und Initiative stark.",  // GEOMANCY
  "Absorbiert die H\204lfte des verursachten Schadens als KP.",  // GIGADRAIN
  "St\201rmt mit aller Kraft auf das Ziel. Der Anwender muss in der n\204chsten Runde aussetzen.",  // GIGAIMPACT
  "Schl\204gt mit einem riesigen Hammer zu. Kann nicht 2-mal hintereinander eingesetzt werden.",  // GIGATONHAMMER
  "Bl\204st eisige Luft auf das Ziel und senkt seine Initiative.",  // GLACIATE
  "In der n\204chsten Runde treffen die Angriffe des Gegners immer und richten doppelten Schaden an.",  // GLAIVERUSH
  "Ein furchterregender Blick, der das Ziel paralysiert.",  // GLARE
  "Bombardiert das Ziel mit Telekinese und errichtet ein Lichtschild.",  // GLITZYGLOW
  "Bringt das Ziel mit Gras zu Fall. Je schwerer das Ziel, desto mehr Schaden.",  // GRASSKNOT
  "Eine angenehme Melodie, die das Ziel einschlafen l\204sst.",  // GRASSWHISTLE
  "Gleitet \201ber den Boden und greift an. Auf Grasfeld greift diese Attacke zuerst an.",  // GRASSYGLIDE
  "Erzeugt 5 Runden lang ein Grasfeld. Pok\202mon am Boden f\201llen am Rundenende KP auf.",  // GRASSYTERRAIN
  "L\204sst einen Apfel aus gro\341er H\224he fallen. Senkt die Verteidigung des Ziels.",  // GRAVAPPLE
  "Ein niedliches Knurren, das den Angriff des Gegners senkt.",  // GROWL
  "Der K\224rper w\204chst rasant. Erh\224ht Angriff und Spezial-Angriff.",  // GROWTH
  "Wird der Anwender durch einen Angriff besiegt, verliert diese Attacke alle AP.",  // GRUDGE
  "Ein brutaler Scherenangriff. Trifft er, wird das Ziel sofort besiegt.",  // GUILLOTINE
  "Bewirft das Ziel mit ekligem M\201ll. Kann vergiften.",  // GUNKSHOT
  "Erzeugt mit den Fl\201geln einen Wirbelsturm und schleudert ihn auf das Ziel.",  // GUST
  "Ein starker Rotationsangriff. Je langsamer der Anwender, desto mehr Schaden.",  // GYROBALL
  "Ein schwerer Schlag mit der Faust. Senkt aber die Initiative des Anwenders.",  // HAMMERARM
  "Spannt die Muskeln an und erh\224ht die Verteidigung.",  // HARDEN
  "Ein Nebel, der die Statusver\204nderungen aller Pok\202mon im Kampf aufhebt.",  // HAZE
  "Eine kr\204ftige Kopfnuss. Ziel schreckt eventuell zur\201ck.",  // HEADBUTT
  "Ein gewaltiger Kopfsto\341. Der Anwender erleidet etwas R\201cksto\341schaden.",  // HEADCHARGE
  "Greift mit aller Kraft an, senkt aber Vert. und Sp.-Vert. des Anwenders.",  // HEADLONGRUSH
  "Ein lebensgef\204hrlicher Kopfsto\341 mit voller Kraft. Der Anwender erleidet schweren R\201cksto\341schaden.",  // HEADSMASH
  "Ein beruhigendes L\204uten heilt die Statusprobleme aller Pok\202mon im Team.",  // HEALBELL
  "Der Anwender wird besiegt. Daf\201r werden KP und Status seines Nachfolgers geheilt.",  // HEALINGWISH
  "Der Anwender ruft seine Freunde zur Heilung. F\201llt bis zur H\204lfte der max. KP auf.",  // HEALORDER
  "Eine heilende Welle f\201llt die KP des Ziels um die H\204lfte seiner max. KP auf.",  // HEALPULSE
  "Eine anmutige Geste mit gewaltigem Schlag. Ziel schreckt eventuell zur\201ck.",  // HEARTSTAMP
  "Je schwerer der Anwender im Vergleich zum Ziel ist, desto mehr Schaden.",  // HEATCRASH
  "Ein sengender Wind. Kann das Ziel verbrennen.",  // HEATWAVE
  "Je schwerer der Anwender im Vergleich zum Ziel ist, desto mehr Schaden.",  // HEAVYSLAM
  "Ein Angriff, der Zielen mit Statusproblemen enormen Schaden zuf\201gt.",  // HEX
  "Ein einzigartiger Angriff, dessen Typ und St\204rke vom Anwender abh\204ngen.",  // HIDDENPOWER
  "Ein verheerender Angriff mit dem ganzen K\224rper.",  // HIGHHORSEPOWER
  "Springt hoch und tritt zu. Bei Fehlschlag nimmt der Anwender Schaden.",  // HIGHJUMPKICK
  "Wetzt die Klauen und erh\224ht Angriff und Genauigkeit.",  // HONECLAWS
  "Greift das Ziel mit einem spitzen Horn an.",  // HORNATTACK
  "Ein Bohrangriff mit dem Horn. Trifft er, wird das Ziel sofort besiegt.",  // HORNDRILL
  "Ein Schlag, der Energie absaugt. Der Anwender f\201llt KP in H\224he der H\204lfte des Schadens auf.",  // HORNLEECH
  "Ein ermutigendes Heulen, das den Angriff erh\224ht.",  // HOWL
  "H\201llt das Ziel in einen heftigen Wirbelsturm. Kann es verwirren.",  // HURRICANE
  "Greift mit einer gro\341en Menge Wasser unter hohem Druck an.",  // HYDROPUMP
  "Ein starker Strahl kochenden Wassers. Bei Sonne steigt seine St\204rke.",  // HYDROSTEAM
  "Ein vernichtender Strahl. Der Anwender muss in der n\204chsten Runde aussetzen.",  // HYPERBEAM
  "Greift mit einem rotierenden spitzen K\224rperteil an. Ignoriert Attacken wie Schutzschild.",  // HYPERDRILL
  "Ein Angriff mit scharfen Z\204hnen. Ziel schreckt eventuell zur\201ck.",  // HYPERFANG
  "Trifft auch, wenn das Ziel Schutzschild, Scanner, Rundumschutz oder Rapidschutz einsetzt.",  // HYPERSPACEHOLE
  "Ein markersch\201tternder Schrei, der dem Ziel Schaden zuf\201gt.",  // HYPERVOICE
  "Hypnose, die das Ziel in tiefen Schlaf versetzt.",  // HYPNOSIS
  "Rollt 5 Runden lang auf das Ziel zu und wird jedes Mal st\204rker.",  // ICEBALL
  "Ein Eisstrahl, der das Ziel einfrieren kann.",  // ICEBEAM
  "Bei\341t mit eisigen Z\204hnen zu. Ziel schreckt eventuell zur\201ck oder wird eingefroren.",  // ICEFANG
  "Ein gewaltiger Schlag. Die Initiative des Anwenders sinkt dabei.",  // ICEHAMMER
  "Ein eisiger Schlag. Kann einfrieren.",  // ICEPUNCH
  "Schleudert Eisbrocken mit hohem Tempo. Diese Attacke greift immer zuerst an.",  // ICESHARD
  "H\201llt die Gliedma\341en in Eis und schl\204gt zu. Zerst\224rt ein aktives Feld.",  // ICESPINNER
  "Schleudert gro\341e Eiszapfen. Ziel schreckt eventuell zur\201ck.",  // ICICLECRASH
  "Schleudert 2- bis 5-mal hintereinander spitze Eiszapfen.",  // ICICLESPEAR
  "Ein eisiger Wind, der die Initiative des Ziels senkt.",  // ICYWIND
  "Die Gegner k\224nnen im Kampf keine Attacken einsetzen, die der Anwender auch kennt.",  // IMPRISON
  "Flammen treffen benachbarte Ziele. Getragene Beeren verbrennen und verschwinden.",  // INCINERATE
  "Ein heftiger Feuersto\341, der das Ziel verbrennt.",  // INFERNO
  "H\204lt das Ziel 4 bis 5 Runden lang fest. W\204hrenddessen kann es nicht fliehen.",  // INFESTATION
  "Schl\204gt Wurzeln und f\201llt jede Runde KP auf, kann aber nicht ausgewechselt werden.",  // INGRAIN
  "Verteilt geladene Teilchen, die Normal-Attacken zu Elektro-Attacken machen.",  // IONDELUGE
  "H\204rtet den K\224rper wie Eisen und erh\224ht die Verteidigung stark.",  // IRONDEFENSE
  "Greift mit seinem eisenharten Kopf an. Ziel schreckt eventuell zur\201ck.",  // IRONHEAD
  "Greift mit einem eisernen Schweif an. Senkt eventuell die Verteidigung des Ziels.",  // IRONTAIL
  "Schl\204gt mit einer Keule zu. Der Typ h\204ngt von der getragenen Maske ab. Hohe Volltrefferquote.",  // IVYCUDGEL
  "Weder Anwender noch Ziel k\224nnen den Kampf verlassen, bis einer von beiden besiegt ist.",  // JAWLOCK
  "H\201llt die Faust in einen Wasserstrom und schl\204gt als Erstes zu.",  // JETPUNCH
  "Sendet unz\204hlige Lichtstrahlen aus. Der Typ h\204ngt von der getragenen Tafel ab.",  // JUDGMENT
  "Springt und tritt zu. Bei Fehlschlag nimmt der Anwender Schaden.",  // JUMPKICK
  "F\201llt die KP des Anwenders und seiner Verb\201ndeten auf und heilt ihre Statusprobleme.",  // JUNGLEHEALING
  "Ein scharfer Handkantenschlag. Hohe Volltrefferquote.",  // KARATECHOP
  "Verbiegt einen L\224ffel, um das Ziel abzulenken. Senkt seine Genauigkeit.",  // KINESIS
  "Sch\201tzt den Anwender vor Schaden. Ber\201hrt ihn ein Gegner, sinkt dessen Angriff stark.",  // KINGSSHIELD
  "Das Ziel kann sein getragenes Item im Kampf nicht mehr nutzen.",  // KNOCKOFF
  "Verbeugt sich, damit das Ziel unvorsichtig wird, und greift dann an. Trifft immer.",  // KOWTOWCLEAVE
  "Sammelt Energie aus der Erdkruste und schadet damit den Gegnern.",  // LANDSWRATH
  "Konzentriert sich, damit der n\204chste Angriff ein Volltreffer wird.",  // LASERFOCUS
  "Kann erst eingesetzt werden, wenn alle anderen Attacken des Pok\202mon eingesetzt wurden.",  // LASTRESORT
  "R\204cht gefallene Kameraden. Je mehr besiegt wurden, desto st\204rker die Attacke.",  // LASTRESPECTS
  "Ein Flammeninferno trifft alle benachbarten Pok\202mon. Kann verbrennen.",  // LAVAPLUME
  "Bewirft den Gegner mit Bl\204ttern.",  // LEAFAGE
  "Schneidet mit einem scharfen Blatt. Hohe Volltrefferquote.",  // LEAFBLADE
  "Ein Sturm aus scharfen Bl\204ttern. Senkt den Spezial-Angriff des Anwenders stark.",  // LEAFSTORM
  "Greift mit scharfen Bl\204ttern an. Senkt eventuell die Genauigkeit des Ziels.",  // LEAFTORNADO
  "Saugt Energie ab. Der Anwender f\201llt KP in H\224he der H\204lfte des Schadens auf.",  // LEECHLIFE
  "Pflanzt Samen ein, die dem Ziel jede Runde KP absaugen und den Anwender heilen.",  // LEECHSEED
  "Ein drohender Blick, der die Verteidigung des Ziels senkt.",  // LEER
  "Leckt das Ziel mit langer Zunge ab. Kann paralysieren.",  // LICK
  "Verspr\201ht geheimnisvolles Wasser, das KP des Anwenders und seiner Verb\201ndeten auff\201llt.",  // LIFEDEW
  "Eine Lichtwand, die 5 Runden lang den Schaden von Spezial-Attacken senkt.",  // LIGHTSCREEN
  "Schl\204gt mit der Kraft des Wassers zu. Senkt eventuell die Verteidigung des Ziels.",  // LIQUIDATION
  "Nimmt das Ziel ins Visier. Der n\204chste Angriff trifft garantiert.",  // LOCKON
  "K\201sst das Ziel mit furchterregender Miene. Trifft er, schl\204ft das Ziel ein.",  // LOVELYKISS
  "Ein tiefer Tritt. Je schwerer das Ziel, desto mehr Schaden.",  // LOWKICK
  "Ein schneller Tritt gegen die Beine, der die Initiative des Ziels senkt.",  // LOWSWEEP
  "Ein Zauberspruch, der verhindert, dass das Ziel Volltreffer landet.",  // LUCKYCHANT
  "Ein seltsames Licht, das den Geist angreift und die Spezial-Verteidigung stark senkt.",  // LUMINACRASH
  "Ein Gebet an den Mond. F\201llt KP auf und heilt Statusprobleme auf der Seite des Anwenders.",  // LUNARBLESSING
  "Der Anwender wird besiegt. Daf\201r werden KP, AP und Status seines Nachfolgers geheilt.",  // LUNARDANCE
  "St\201rzt sich mit aller Kraft auf das Ziel und senkt seinen Angriff.",  // LUNGE
  "Ein greller Lichtblitz. Senkt eventuell die Spezial-Verteidigung des Ziels.",  // LUSTERPURGE
  "Ein blitzschneller Schlag, der immer zuerst trifft.",  // MACHPUNCH
  "Verstreut seltsame Bl\204tter, die das Ziel verfolgen. Trifft immer.",  // MAGICALLEAF
  "Das Ziel wird 2 bis 5 Runden lang in einem Feuersturm gefangen.",  // MAGMASTORM
  "Wirft Eisenbomben, die am Gegner haften. Trifft immer.",  // MAGNETBOMB
  "Schwebt 5 Runden lang durch ein elektrisch erzeugtes Magnetfeld.",  // MAGNETRISE
  "Ein Erdbeben von wechselnder St\204rke, das alle benachbarten Pok\202mon trifft.",  // MAGNITUDE
  "Wirft viele M\201nzen, senkt aber den Spezial-Angriff. Erh\224ht das Preisgeld.",  // MAKEITRAIN
  "Fesselt das Ziel mit einer Kette und raubt ihm Kraft. Kann es schwer vergiften.",  // MALIGNANTCHAIN
  "Bespritzt das Ziel mit Tee und f\201llt KP in H\224he der H\204lfte des Schadens auf. Kann verbrennen.",  // MATCHAGOTCHA
  "Hindert das Ziel mit finsterem Blick an der Flucht.",  // MEANLOOK
  "Der Anwender meditiert und erh\224ht seinen Angriff.",  // MEDITATE
  "Absorbiert die H\204lfte des verursachten Schadens als KP.",  // MEGADRAIN
  "Ein heftiger Rammsto\341 mit gewaltigen H\224rnern.",  // MEGAHORN
  "Ein Tritt mit enormer Kraft.",  // MEGAKICK
  "Ein Schlag von gro\341er St\204rke.",  // MEGAPUNCH
  "Der Anwender wird besiegt, senkt aber Angriff und Spezial-Angriff des Ziels stark.",  // MEMENTO
  "Gibt den zuletzt erlittenen Angriff mit viel gr\224\341erer Wucht an das Ziel zur\201ck.",  // METALBURST
  "Ein Angriff mit Stahlklauen. Erh\224ht eventuell den Angriff des Anwenders.",  // METALCLAW
  "Ein gr\204ssliches Metallger\204usch, das die Sp.-Vert. des Ziels stark senkt.",  // METALSOUND
  "Schl\204gt mit seiner Lauchstange zu. Der Anwender muss danach 1 Runde aussetzen.",  // METEORASSAULT
  "Runde 1: erh\224ht den Sp.-Ang. Runde 2: greift das Ziel an.",  // METEORBEAM
  "Ein Schlag wie ein Meteor. Erh\224ht eventuell den Angriff des Anwenders.",  // METEORMASH
  "Schneidet das Ziel mit Licht aus seinem Kopf. Trifft auch, wenn es sich sch\201tzt.",  // MIGHTYCLEAVE
  "F\201llt die H\204lfte der max. KP auf. Au\341erhalb des Kampfes kann es KP an Verb\201ndete abgeben.",  // MILKDRINK
  "L\204sst den Kopf explodieren und greift alle um sich herum an. Der Anwender nimmt auch Schaden.",  // MINDBLOWN
  "Erahnt die Bewegungen des Ziels. Der n\204chste Angriff trifft garantiert.",  // MINDREADER
  "Der Anwender schrumpft und erh\224ht seinen Ausweichwert stark.",  // MINIMIZE
  "Erm\224glicht es, Unlicht-Pok\202mon mit jeder Attacke zu treffen, und trifft ausweichende Gegner.",  // MIRACLEEYE
  "Kontert Spezial-Angriffe mit doppeltem Schaden.",  // MIRRORCOAT
  "Kontert mit der zuletzt vom Ziel eingesetzten Attacke.",  // MIRRORMOVE
  "Setzt einen Energieblitz aus seinem polierten K\224rper frei. Senkt eventuell die Genauigkeit.",  // MIRRORSHOT
  "H\201llt den Anwender in Nebel und sch\201tzt 5 Runden lang die Statuswerte seines Teams.",  // MIST
  "Eine Nebelwolke umh\201llt das Ziel. Senkt eventuell seinen Spezial-Angriff.",  // MISTBALL
  "Ein mysteri\224ser Nebel. Auf Nebelfeld greift diese Attacke fr\201her an.",  // MISTYEXPLOSION
  "H\201llt den Boden 5 Runden lang in Nebel. Pok\202mon am Boden sind vor Statusproblemen gesch\201tzt.",  // MISTYTERRAIN
  "Greift mit der Kraft des Mondes an. Senkt eventuell den Spezial-Angriff des Ziels.",  // MOONBLAST
  "Ein mysteri\224ser Strahl aus Mondenergie. Ignoriert die F\204higkeit des Ziels.",  // MOONGEISTBEAM
  "F\201llt KP des Anwenders auf. Die Menge h\204ngt vom Wetter ab.",  // MOONLIGHT
  "F\201llt KP des Anwenders auf. Die Menge h\204ngt vom Wetter ab.",  // MORNINGSUN
  "Ein Rotationsangriff, der das Ziel vergiftet und Fessel-Effekte sowie Egelsamen entfernt.",  // MORTALSPIN
  "Greift mit eisbergro\341en Eiszapfen an. Ziel schreckt eventuell zur\201ck.",  // MOUNTAINGALE
  "Wirft eine harte Schlammkugel. Senkt eventuell die Genauigkeit des Ziels.",  // MUDBOMB
  "Greift mit schlammigem Wasser an. Senkt eventuell die Genauigkeit.",  // MUDDYWATER
  "Bewirft das Ziel mit Lehm und senkt seine Initiative.",  // MUDSHOT
  "Wirft dem Ziel Lehm ins Gesicht und senkt seine Genauigkeit.",  // MUDSLAP
  "H\201llt sich in Lehm und schw\204cht Elektro-Attacken, solange er im Kampf ist.",  // MUDSPORT
  "Greift mit gewaltiger Energie an. Der Typ h\204ngt von der getragenen Disc ab.",  // MULTIATTACK
  "Speit eine gl\201hend hei\341e Flamme und senkt den Spezial-Angriff des Ziels.",  // MYSTICALFIRE
  "Entfesselt eine mysteri\224se Kraft und erh\224ht zudem den Spezial-Angriff des Anwenders.",  // MYSTICALPOWER
  "Regt das Hirn mit b\224sen Gedanken an und erh\224ht den Spezial-Angriff drastisch.",  // NASTYPLOT
  "Die getragene Beere verleiht Kraft. Typ und St\204rke h\204ngen von der Beere ab.",  // NATURALGIFT
  "Trifft das Ziel mit dem Zorn der Natur und halbiert seine KP.",  // NATURESMADNESS
  "Schl\204gt mit stacheligen Armen zu. Ziel schreckt eventuell zur\201ck.",  // NEEDLEARM
  "Eine pechschwarze Welle trifft das Ziel. Senkt eventuell seine Genauigkeit.",  // NIGHTDAZE
  "Erzeugt eine Illusion. Schaden in H\224he des Levels des Anwenders.",  // NIGHTSHADE
  "Schl\204gt bei der ersten Gelegenheit zu. Hohe Volltrefferquote.",  // NIGHTSLASH
  "Ein Kampfschrei, der Angriff und Spezial-Angriff des Gegners senkt.",  // NOBLEROAR
  "Erh\224ht alle Statuswerte, aber der Anwender kann nicht mehr fliehen oder ausgewechselt werden.",  // NORETREAT
  "Reibt seine geladenen Backen am Ziel und paralysiert es.",  // NUZZLE
  "Absorbiert Energie vom Ziel und f\201llt KP in H\224he von 3/4 des Schadens auf.",  // OBLIVIONWING
  "Sch\201tzt vor Angriffen. Ber\201hrt ihn ein Gegner, sinkt dessen Verteidigung stark. Schl\204gt bei h\204ufigem Einsatz fehl.",  // OBSTRUCT
  "Spritzt Tinte ins Gesicht. Senkt eventuell die Genauigkeit.",  // OCTAZOOKA
  "H\204lt das Ziel fest, sodass es nicht fliehen kann, und senkt jede Runde Vert. und Sp.-Vert.",  // OCTOLOCK
  "Erm\224glicht es, Geist-Pok\202mon mit jeder Attacke zu treffen, und trifft ausweichende Gegner.",  // ODORSLEUTH
  "Ein schauriger Wind. Erh\224ht eventuell alle Statuswerte des Anwenders.",  // OMINOUSWIND
  "Hat er ein Nigiragi im Maul, steigt je nach dessen Form ein Statuswert.",  // ORDERUP
  "Greift mit unz\204hligen bl\204ulichen Lichtstrahlen an.",  // ORIGINPULSE
  "Greift 2 bis 3 Runden lang an. Danach ist der Anwender verwirrt.",  // OUTRAGE
  "Schl\204gt auf seinen K\224rper wie auf Saiten und greift mit gewaltigen Vibrationen an.",  // OVERDRIVE
  "Ein Angriff mit voller Kraft, der den Sp.-Ang. des Anwenders stark senkt.",  // OVERHEAT
  "Addiert die KP von Anwender und Ziel und teilt sie gleichm\204\341ig auf.",  // PAINSPLIT
  "Schadet allen Pok\202mon in der N\204he und absorbiert die H\204lfte des Schadens als KP.",  // PARABOLICCHARGE
  "Senkt Angriff und Spezial-Angriff des Ziels und tauscht dann mit einem Team-Pok\202mon.",  // PARTINGSHOT
  "Doppelte St\204rke, wenn das Ziel vor dem Anwender angreift.",  // PAYBACK
  "Wirft M\201nzen auf das Ziel. Nach dem Kampf erh\204lt man Geld.",  // PAYDAY
  "Sticht das Ziel mit einem spitzen Horn oder Schnabel.",  // PECK
  "Ein heftiger Bl\201tensturm, der allen Pok\202mon in der N\204he schadet.",  // PETALBLIZZARD
  "Greift 2 bis 3 Runden lang mit Bl\201ten an. Danach ist der Anwender verwirrt.",  // PETALDANCE
  "Verschwindet in Runde 1 und greift in Runde 2 an. Trifft auch, wenn das Ziel sich sch\201tzt.",  // PHANTOMFORCE
  "Vergleicht Angriff und Spezial-Angriff und richtet mit dem h\224heren Wert Schaden an.",  // PHOTONGEYSER
  "Schie\341t spitze Stacheln, die 2- bis 5-mal treffen.",  // PINMISSILE
  "Greift mit den F\204usten an. Macht Normal-Attacken zu Elektro-Attacken.",  // PLASMAFISTS
  "Freundet sich mit dem Gegner an, nimmt ihm die Kampflust und senkt seinen Angriff.",  // PLAYNICE
  "Tollt wild mit dem Ziel herum. Senkt eventuell seinen Angriff.",  // PLAYROUGH
  "Pickt das Ziel. Tr\204gt es eine Beere, isst der Anwender sie und erh\204lt ihren Effekt.",  // PLUCK
  "Bei\341t mit giftigen Z\204hnen zu. Kann das Ziel schwer vergiften.",  // POISONFANG
  "Spr\201ht dem Ziel eine Giftgaswolke ins Gesicht. Kann vergiften.",  // POISONGAS
  "Sticht mit giftigem Tentakel oder Arm zu. Kann vergiften.",  // POISONJAB
  "Verstreut giftigen Staub. Kann das Ziel vergiften.",  // POISONPOWDER
  "Sticht mit einem Giftstachel zu. Kann das Ziel vergiften.",  // POISONSTING
  "Ein Schweifangriff, der vergiften kann. Hohe Volltrefferquote.",  // POISONTAIL
  "Wirft eine explosive Kugel. Trifft sie einen Verb\201ndeten, f\201llt sie dessen KP auf.",  // POLLENPUFF
  "Greift mit dem Item des Ziels an. Tr\204gt es keines, schl\204gt die Attacke fehl.",  // POLTERGEIST
  "Ein Gruppenangriff, der das Ziel 1- bis 10-mal hintereinander trifft.",  // POPULATIONBOMB
  "Springt das Ziel an und senkt seine Initiative.",  // POUNCE
  "Schl\204gt mit Schweif, Bein oder \216hnlichem zu.",  // POUND
  "Ein eisiger Pulverschnee. Kann einfrieren.",  // POWDERSNOW
  "Ein Lichtstrahl, der funkelt, als best\201nde er aus tausend Juwelen.",  // POWERGEM
  "Je mehr die Statuswerte des Anwenders erh\224ht wurden, desto st\204rker die Attacke.",  // POWERTRIP
  "Mit jedem Treffer h\204rten die F\204uste. Trifft er, steigt der Angriff des Anwenders.",  // POWERUPPUNCH
  "Peitscht das Ziel heftig mit Ranken oder Tentakeln.",  // POWERWHIP
  "Verwandelt die Kraft der Erde in scharfe Klingen und greift an.",  // PRECIPICEBLADES
  "Ein Geschenk mit Sprengfalle. Manchmal f\201llt es stattdessen KP auf.",  // PRESENT
  "Feuert einen starken Strahl durch ein Prisma. Der Anwender muss danach 1 Runde aussetzen.",  // PRISMATICLASER
  "Wehrt Angriffe ab, kann aber bei wiederholtem Einsatz fehlschlagen.",  // PROTECT
  "Ein seltsamer Strahl. Kann verwirren.",  // PSYBEAM
  "Zerschneidet das Ziel mit einem \204therischen Schwert. Auf Elektrofeld st\204rker.",  // PSYBLADE
  "Ein starker telekinetischer Angriff. Senkt eventuell die Sp.-Vert. des Ziels.",  // PSYCHIC
  "Psycho-Kraft, die auch Barrieren wie Lichtschild und Reflektor zerst\224rt.",  // PSYCHICFANGS
  "5 Runden lang werden Psycho-Attacken verst\204rkt und Erstschlag-Attacken schlagen fehl.",  // PSYCHICTERRAIN
  "Ein Angriff mit voller Kraft, der den Sp.-Ang. des Anwenders stark senkt.",  // PSYCHOBOOST
  "Schneidet mit Klingen aus Psycho-Energie. Hohe Volltrefferquote.",  // PSYCHOCUT
  "Hypnotisiert sich selbst und kopiert die Statusver\204nderungen des Ziels.",  // PSYCHUP
  "Greift in Psycho-Energie geh\201llt an und erh\224ht zudem die Verteidigung.",  // PSYSHIELDBASH
  "Eine Psycho-Welle, die dem Ziel gro\341en physischen Schaden zuf\201gt.",  // PSYSHOCK
  "Eine seltsame Psycho-Welle, die dem Ziel physischen Schaden zuf\201gt.",  // PSYSTRIKE
  "Eine Energiewelle von wechselnder St\204rke.",  // PSYWAVE
  "Je mehr das Ziel seine Statuswerte erh\224ht hat, desto st\204rker die Attacke.",  // PUNISHMENT
  "Heilt das Statusproblem des Ziels und f\201llt daf\201r die eigenen KP auf.",  // PURIFY
  "Doppelter Schaden gegen ein Ziel, das gerade ausgewechselt wird.",  // PURSUIT
  "Entz\201ndet einen kleinen Stein und greift mit dem Feuerball an. Kann verbrennen.",  // PYROBALL
  "Ein schneller Angriff, bei dem der Anwender zuerst zuschl\204gt.",  // QUICKATTACK
  "Sch\201tzt das Team vor Erstschlag-Attacken. Kann bei wiederholtem Einsatz fehlschlagen.",  // QUICKGUARD
  "Ein mystischer Tanz, der Spezial-Angriff, Spezial-Verteidigung und Initiative erh\224ht.",  // QUIVERDANCE
  "Solange aktiv, steigt der Angriff des Anwenders bei jedem erlittenen Treffer.",  // RAGE
  "Verwandelt Wut in Energie. Je mehr Treffer der Anwender erlitten hat, desto st\204rker.",  // RAGEFIST
  "Der Typ h\204ngt von der Form des Anwenders ab. Zerst\224rt Barrieren.",  // RAGINGBULL
  "Greift 2 bis 3 Runden lang mit heftigen Flammen an. Danach ist der Anwender verwirrt.",  // RAGINGFURY
  "Ein starker Regen verst\204rkt 5 Runden lang Wasser-Attacken.",  // RAINDANCE
  "Ein Rotationsangriff, der die Initiative erh\224ht und Effekte wie Klammergriff, Egelsamen oder Stachler entfernt.",  // RAPIDSPIN
  "Schneidet mit scharfen Bl\204ttern. Hohe Volltrefferquote.",  // RAZORLEAF
  "Schneidet mit einer scharfen Muschel. Senkt eventuell die Verteidigung des Ziels.",  // RAZORSHELL
  "Schneidende Windklingen greifen an. Hohe Volltrefferquote.",  // RAZORWIND
  "F\201llt bis zur H\204lfte der max. KP auf.",  // RECOVER
  "Recycelt ein bereits verbrauchtes Item zum einmaligen Gebrauch, sodass es wieder nutzbar ist.",  // RECYCLE
  "Eine Lichtwand, die 5 Runden lang den Schaden physischer Angriffe senkt.",  // REFLECT
  "Ruht sich aus und heilt Paralyse, Vergiftung oder Verbrennung.",  // REFRESH
  "Ein uralter Gesang r\201hrt die Gegner in der N\204he. Kann sie einschl\204fern.",  // RELICSONG
  "F\201llt KP auf und heilt Statusprobleme. Der Anwender schl\204ft die n\204chsten 2 Runden.",  // REST
  "Wurde in der Vorrunde ein Teammitglied besiegt, steigt die St\204rke.",  // RETALIATE
  "Je gr\224\341er die Freundschaft zum Trainer, desto st\204rker ist diese Attacke.",  // RETURN
  "Ein energischer Tanz vom prim\204ren Typ des Anwenders. Hat er ihn verloren, vom sekund\204ren.",  // REVELATIONDANCE
  "Doppelter Schaden, wenn der Anwender in derselben Runde getroffen wurde.",  // REVENGE
  "Ein verzweifelter Angriff. Je weniger KP der Anwender hat, desto st\204rker.",  // REVERSAL
  "Doppelte St\204rke, wenn das Ziel auf einem Elektrofeld steht.",  // RISINGVOLTAGE
  "Verscheucht das Ziel und zwingt es zum Tausch. Gegen wilde Pok\202mon endet der Kampf.",  // ROAR
  "Eine Kraft, die sogar die Zeit verzerrt. Der Anwender muss in der n\204chsten Runde aussetzen.",  // ROAROFTIME
  "Wirft 2- bis 5-mal hintereinander harte Felsen.",  // ROCKBLAST
  "Ein gewaltiger Sturmangriff. Kann das Ziel verwirren.",  // ROCKCLIMB
  "Poliert den K\224rper, um den Luftwiderstand zu senken. Erh\224ht die Initiative stark.",  // ROCKPOLISH
  "Wirft gro\341e Felsen. Ziel schreckt eventuell zur\201ck.",  // ROCKSLIDE
  "Schl\204ge, die Felsen zertr\201mmern. Senkt eventuell die Verteidigung des Ziels.",  // ROCKSMASH
  "Wirft einen kleinen Stein auf das Ziel.",  // ROCKTHROW
  "Wirft Felsen, die das Ziel aufhalten und seine Initiative senken.",  // ROCKTOMB
  "Schleudert einen riesigen Felsbrocken. Der Anwender muss in der n\204chsten Runde aussetzen.",  // ROCKWRECKER
  "Ein schneller Drehtritt. Ziel schreckt eventuell zur\201ck.",  // ROLLINGKICK
  "Rollt sich ein und rollt wie ein Rad \201ber das Kampffeld.",  // ROLLOUT
  "Landet zum Ausruhen auf dem Boden. F\201llt bis zur H\204lfte der max. KP auf.",  // ROOST
  "Ein Gesang als Angriff. Je mehr Pok\202mon ihn einsetzen, desto st\204rker wird er.",  // ROUND
  "Eine verheerende Katastrophe, die die KP des Ziels halbiert.",  // RUINATION
  "Eine mystische Flamme von gro\341er Hitze. Kann verbrennen.",  // SACREDFIRE
  "Greift mit einem Schwert an und ignoriert die Statusver\204nderungen des Ziels.",  // SACREDSWORD
  "Errichtet ein Schutzfeld, das 5 Runden lang Statusprobleme verhindert.",  // SAFEGUARD
  "Das Ziel verliert jede Runde KP. Stahl- und Wasser-Pok\202mon trifft es besonders hart.",  // SALTCURE
  "Wirft dem Ziel Sand ins Gesicht und senkt seine Genauigkeit.",  // SANDATTACK
  "H\201llt das Ziel in gl\201hend hei\341en Sand. Kann verbrennen.",  // SANDSEARSTORM
  "Ein Sandsturm, der 5 Runden dauert. Schadet allen Pok\202mon au\341er Gestein-, Boden- und Stahl-Pok\202mon.",  // SANDSTORM
  "F\204ngt das Ziel 4 bis 5 Runden lang in einem Sandwirbel.",  // SANDTOMB
  "Greift mit verstreuten Samen an und pflanzt Egelsamen ein.",  // SAPPYSEED
  "Bespritzt das Ziel mit kochendem Wasser. Kann verbrennen.",  // SCALD
  "Schie\341t 2- bis 5-mal Schuppen ab. Erh\224ht die Initiative des Anwenders, senkt aber seine Verteidigung.",  // SCALESHOT
  "Erschreckt das Ziel und senkt seine Initiative stark.",  // SCARYFACE
  "Bewirft das Ziel mit gl\201hend hei\341em Sand. Kann verbrennen.",  // SCORCHINGSANDS
  "Kratzt mit scharfen Klauen.",  // SCRATCH
  "Ein schriller Schrei, der die Verteidigung des Ziels stark senkt.",  // SCREECH
  "Ein Flammeninferno trifft alle Pok\202mon in der N\204he. Kann verbrennen.",  // SEARINGSHOT
  "Ein Angriff mit geheimer Kraft. Der Zusatzeffekt h\204ngt von der Umgebung ab.",  // SECRETPOWER
  "Spie\341t das Ziel mit einem S\204bel auf und verletzt es mit mystischer Kraft.",  // SECRETSWORD
  "L\204sst eine Salve explosiver Samen von oben auf das Ziel regnen.",  // SEEDBOMB
  "Setzt eine Schockwelle aus dem K\224rper frei. Senkt eventuell die Sp.-Vert. des Ziels.",  // SEEDFLARE
  "Wirft das Ziel mit Schwerkraft zu Boden. Schaden in H\224he des Levels des Anwenders.",  // SEISMICTOSS
  "Der Anwender explodiert und schadet allen Pok\202mon um sich herum. Er wird sofort besiegt.",  // SELFDESTRUCT
  "Schleudert eine dunkle Kugel. Senkt eventuell die Sp.-Vert. des Ziels.",  // SHADOWBALL
  "Schl\204gt mit einem von einem Geist besessenen Knochen zu. Senkt eventuell die Verteidigung.",  // SHADOWBONE
  "Schl\204gt mit einer Klaue aus Schatten zu. Hohe Volltrefferquote.",  // SHADOWCLAW
  "Verschwindet in Runde 1 und trifft in Runde 2. Trifft auch, wenn das Ziel sich sch\201tzt.",  // SHADOWFORCE
  "Ein Schlag aus den Schatten, der immer trifft.",  // SHADOWPUNCH
  "Streckt seinen Schatten aus und greift von hinten an. Diese Attacke greift immer zuerst an.",  // SHADOWSNEAK
  "Der Anwender sch\204rft seine Kanten und erh\224ht seinen Angriff.",  // SHARPEN
  "Ein Angriff mit eisiger K\204lte. Trifft er, wird das Ziel sofort besiegt.",  // SHEERCOLD
  "Zerbricht die Schale. Senkt Vert. und Sp.-Vert., erh\224ht aber Angriff, Sp.-Ang. und Initiative stark.",  // SHELLSMASH
  "Trifft ihn ein physischer Angriff, explodiert die Panzerfalle und schadet dem Angreifer.",  // SHELLTRAP
  "Ein blitzschneller Elektroangriff, der immer trifft.",  // SHOCKWAVE
  "F\201llt die H\204lfte der max. KP auf. Bei Sandsturm mehr.",  // SHOREUP
  "Ein unheimlicher Lichtstrahl. Kann das Ziel verwirren.",  // SIGNALBEAM
  "Eine Seidenfalle sch\201tzt den Anwender. Senkt die Initiative jedes Angreifers, der sie ber\201hrt.",  // SILKTRAP
  "Ein Wind voller Schuppenstaub. Erh\224ht eventuell alle Statuswerte des Anwenders.",  // SILVERWIND
  "Ein sanftes Wiegenlied, das das Ziel in tiefen Schlaf versetzt.",  // SING
  "H\201llt sich in Feuer und st\201rmt auf das Ziel. Verbrennt es immer.",  // SIZZLYSLIDE
  "Erlernt dauerhaft die letzte Attacke des Ziels. Kann nur einmal eingesetzt werden.",  // SKETCH
  "Greift das Ziel heimlich von hinten an und senkt seinen Spezial-Angriff.",  // SKITTERSMACK
  "Runde 1: zieht den Kopf ein und erh\224ht die Verteidigung. Runde 2: Kopfsto\341.",  // SKULLBASH
  "Ein Angriff \201ber 2 Runden mit hoher Volltrefferquote. Ziel schreckt eventuell zur\201ck.",  // SKYATTACK
  "Ein wuchtiger Aufw\204rtshaken.",  // SKYUPPERCUT
  "Der Anwender faulenzt und f\201llt die H\204lfte seiner max. KP auf.",  // SLACKOFF
  "Schl\204gt mit Schweif, Ranken oder \216hnlichem zu.",  // SLAM
  "Greift mit Klingen oder Scheren an. Hohe Volltrefferquote.",  // SLASH
  "Verstreut Staub, der das Ziel einschlafen l\204sst.",  // SLEEPPOWDER
  "Bewirft das Ziel mit Schlamm. Kann vergiften.",  // SLUDGE
  "Bewirft das Ziel mit Schlamm. Kann vergiften.",  // SLUDGEBOMB
  "Eine giftige Welle, die Pok\202mon in der N\204he schadet. Kann vergiften.",  // SLUDGEWAVE
  "Wirft einen Stein oder ein Geschoss. Fliegende Ziele st\201rzen zu Boden.",  // SMACKDOWN
  "Spie\341t den Gegner mit seinem spitzen Horn auf. Trifft immer.",  // SMARTSTRIKE
  "Doppelter Schaden gegen paralysierte Ziele, heilt aber auch ihre Paralyse.",  // SMELLINGSALTS
  "Greift mit giftigen Gasen an. Kann vergiften.",  // SMOG
  "Senkt die Genauigkeit des Ziels mit Rauch oder Tinte.",  // SMOKESCREEN
  "F\204ngt das Ziel 4 bis 5 Runden lang in einem Fangeisen.",  // SNAPTRAP
  "Ein unertr\204gliches Gebr\201ll, das den Spezial-Angriff der Ziele senkt.",  // SNARL
  "Greift an und ignoriert F\204higkeiten oder Attacken, die Angriffe umlenken.",  // SNIPESHOT
  "Lautes Schnarchen, nur im Schlaf einsetzbar. Ziel schreckt eventuell zur\201ck.",  // SNORE
  "L\204sst es 5 Runden lang schneien. Erh\224ht die Verteidigung von Eis-Pok\202mon.",  // SNOWSCAPE
  "Ein Wasserschwall macht das Ziel zum Wasser-Pok\202mon.",  // SOAK
  "F\201llt die H\204lfte der max. KP auf. Au\341erhalb des Kampfes gibt es KP an Verb\201ndete ab.",  // SOFTBOILED
  "Runde 1: sammelt Licht. Runde 2: Angriff.",  // SOLARBEAM
  "Sammelt in Runde 1 Licht und greift in Runde 2 mit einer Lichtklinge an.",  // SOLARBLADE
  "Schockwellen, die dem Ziel immer 20 KP abziehen.",  // SONICBOOM
  "Zerrei\341t das Ziel und den Raum um es herum. Hohe Volltrefferquote.",  // SPACIALREND
  "Ein Elektro-Rammangriff. Kann paralysieren.",  // SPARK
  "Singt und setzt Blasen frei. Getroffene Pok\202mon werden von Verbrennungen geheilt.",  // SPARKLINGARIA
  "H\201llt das Ziel in einen Wirbel. Heilt alle Statusprobleme im Team.",  // SPARKLYSWIRL
  "Versteckt sich im Schatten des Ziels, stiehlt dessen Statuswerterh\224hungen und greift an.",  // SPECTRALTHIEF
  "Fesselt das Ziel in klebriger Seide, sodass es nicht fliehen kann.",  // SPIDERWEB
  "Schie\341t spitze Stacheln, die 2- bis 5-mal treffen.",  // SPIKECANNON
  "Verteilt Stacheln auf der Gegnerseite. Sie schaden Gegnern, die eingewechselt werden.",  // SPIKES
  "Wehrt Angriffe ab und schadet Gegnern, die den Anwender ber\201hren.",  // SPIKYSHIELD
  "Dreht sich heftig. Senkt die Initiative des Anwenders stark.",  // SPINOUT
  "Ein so wuchtiger Angriff, dass er die Moral des Ziels bricht. Senkt seinen Spezial-Angriff.",  // SPIRITBREAK
  "Greift an und heftet den Schatten des Ziels an den Boden. Es kann nicht fliehen.",  // SPIRITSHACKLE
  "L\204sst seinem Groll freien Lauf. Die letzte Attacke des Ziels verliert 4 AP.",  // SPITE
  "Setzt die mit Horter gesammelte Energie auf einmal frei.",  // SPITUP
  "Hat keinerlei Wirkung. Es platscht nur herum...",  // SPLASH
  "Verstreut Sporen, die einschl\204fern.",  // SPORE
  "Ein Sturm aus Liebe und Hass umh\201llt das Ziel. Senkt eventuell seinen Angriff.",  // SPRINGTIDESTORM
  "Eine schwebende Steinfalle, die eingewechselten Gegnern schadet.",  // STEALTHROCK
  "H\201llt den Gegner in gl\201hend hei\341en Dampf. Kann verbrennen.",  // STEAMERUPTION
  "Rollt sich zur Kugel und \201berrollt das Ziel. Ziel schreckt eventuell zur\201ck.",  // STEAMROLLER
  "Feuert einen Strahl aus dem Metall seines K\224rpers. Der Anwender nimmt dabei Schaden.",  // STEELBEAM
  "Greift an und zerst\224rt das aktive Feld. Schl\204gt fehl, wenn keines aktiv ist.",  // STEELROLLER
  "Harte Fl\201gel treffen das Ziel. Erh\224ht eventuell die Verteidigung des Anwenders.",  // STEELWING
  "Spannt ein klebriges Netz auf der Gegnerseite. Senkt die Initiative eingewechselter Pok\202mon.",  // STICKYWEB
  "Sammelt Kraft und erh\224ht Verteidigung und Spezial-Verteidigung. Bis zu 3-mal einsetzbar.",  // STOCKPILE
  "Ein gewaltiges Stampfen. Ziel schreckt eventuell zur\201ck.",  // STOMP
  "Greift aus Frust an. Doppelte St\204rke, wenn die letzte Attacke fehlschlug.",  // STOMPINGTANTRUM
  "Greift mit einer Steinaxt an. Die Splitter bleiben um das Ziel herum liegen.",  // STONEAXE
  "Rammt dem Ziel scharfe Steine in den Leib. Hohe Volltrefferquote.",  // STONEEDGE
  "Je mehr die Statuswerte des Anwenders erh\224ht wurden, desto mehr Schaden.",  // STOREDPOWER
  "Ein vernichtender Schlag. Immer ein Volltreffer.",  // STORMTHROW
  "Greift mit ausgesto\341enem Dampf an. Kann das Ziel verwirren.",  // STRANGESTEAM
  "Ein kr\204ftiger Schlag.",  // STRENGTH
  "Senkt den Angriff des Ziels und f\201llt KP in H\224he von dessen Angriffswert auf.",  // STRENGTHSAP
  "Bespinnt das Ziel mit Seide und senkt seine Initiative.",  // STRINGSHOT
  "Wehrt sich und senkt zudem den Spezial-Angriff der Gegner.",  // STRUGGLEBUG
  "Verstreut Staub, der das Ziel paralysiert.",  // STUNSPORE
  "Rei\341t das Ziel zu Boden. Schadet auch dem Anwender.",  // SUBMISSION
  "Greift zuerst an. Schl\204gt fehl, wenn das Ziel keinen Angriff vorbereitet.",  // SUCKERPUNCH
  "Verst\204rkt 5 Runden lang die Sonne und damit Feuer-Attacken.",  // SUNNYDAY
  "Greift das Ziel mit der Wucht eines Meteors an.",  // SUNSTEELSTRIKE
  "Elektrisiert den K\224rper und wirft sich auf das Ziel. Bei Fehlschlag nimmt der Anwender Schaden.",  // SUPERCELLSLAM
  "Scharfe Z\204hne halbieren die KP des Ziels.",  // SUPERFANG
  "Ein sehr starker Angriff, der aber Angriff und Verteidigung des Anwenders senkt.",  // SUPERPOWER
  "Seltsame Schallwellen, die das Ziel verwirren k\224nnen.",  // SUPERSONIC
  "\232berflutet das Kampffeld mit einer Riesenwelle.",  // SURF
  "Verwirrt das Ziel, erh\224ht aber auch seinen Angriff stark.",  // SWAGGER
  "Absorbiert die mit Horter gesammelte Energie und f\201llt KP auf.",  // SWALLOW
  "Ein so s\201\341er Kuss, dass das Ziel verwirrt wird.",  // SWEETKISS
  "Ein s\201\341er Duft senkt den Ausweichwert des Gegners. Lockt auch wilde Pok\202mon an.",  // SWEETSCENT
  "Feuert sternf\224rmige Strahlen, die immer treffen.",  // SWIFT
  "Tauscht blitzschnell die getragenen Items mit dem Ziel.",  // SWITCHEROO
  "Ein wilder Tanz, der den Angriff stark erh\224ht.",  // SWORDSDANCE
  "Eine seltsame Welle, die allen benachbarten Pok\202mon desselben Typs wie der Anwender schadet.",  // SYNCHRONOISE
  "F\201llt KP des Anwenders auf. Die Menge h\204ngt vom Wetter ab.",  // SYNTHESIS
  "\232berzieht das Ziel mit z\204hem Nektar und senkt 3 Runden lang seine Initiative.",  // SYRUPBOMB
  "Eine Salve aus Teilchenklingen, die 2-mal hintereinander trifft. Trifft immer.",  // TACHYONCUTTER
  "Der Anwender rammt das Ziel mit dem ganzen K\224rper.",  // TACKLE
  "Konzentriert sich auf einen Lichtblitz und erh\224ht den Spezial-Angriff drastisch.",  // TAILGLOW
  "Schl\204gt 2- bis 5-mal hintereinander mit dem Schweif oder anderen K\224rperteilen zu.",  // TAILSLAP
  "Wedelt niedlich mit dem Schweif und senkt die Verteidigung des Gegners.",  // TAILWHIP
  "Ein starker R\201ckenwind verdoppelt 4 Runden lang die Initiative der Pok\202mon im Team.",  // TAILWIND
  "Ein ungest\201mer Rammangriff. Der Anwender erleidet R\201cksto\341schaden.",  // TAKEDOWN
  "Heilt die eigenen Statusprobleme und erh\224ht Sp.-Ang. und Sp.-Vert.",  // TAKEHEART
  "Bewirft das Ziel mit Teer. Senkt seine Initiative und macht es anf\204lliger f\201r Feuer-Attacken.",  // TARSHOT
  "Reizt das Ziel, sodass es 3 Runden lang nur Angriffsattacken einsetzen kann.",  // TAUNT
  "Nimmt dem Ziel mit einem Blick die Kampflust und senkt Angriff und Spezial-Angriff.",  // TEARFULLOOK
  "Feuert einen gro\341en Laser. Der Typ h\204ngt vom getragenen Modul ab.",  // TECHNOBLAST
  "Ein wackeliger Tanz, der alle Pok\202mon um den Anwender herum verwirrt.",  // TEETERDANCE
  "Setzt Terapagos es in seiner Stellarform ein, trifft es alle Gegner.",  // TERASTARSTORM
  "Nutzt die Energie des aktiven Feldes. Das Feld bestimmt Typ und St\204rke.",  // TERRAINPULSE
  "Greift an und stiehlt das Item des Ziels. Tr\204gt der Anwender schon ein Item, stiehlt er nichts.",  // THIEF
  "Trifft auch Pok\202mon in der Luft und holt sie zu Boden.",  // THOUSANDARROWS
  "Bebenwellen breiten sich zu allen Gegnern aus. Getroffene k\224nnen nicht fliehen.",  // THOUSANDWAVES
  "Greift 2 bis 3 Runden lang an. Danach ist der Anwender verwirrt.",  // THRASH
  "Ein so heftiger Schmerz, dass das Ziel 2 Runden lang keine L\204rm-Attacken einsetzen kann.",  // THROATCHOP
  "Ein m\204chtiger Blitz, der dem Ziel schadet. Kann paralysieren.",  // THUNDER
  "Ein starker Elektroangriff. Kann das Ziel paralysieren.",  // THUNDERBOLT
  "Sperrt das Ziel 4 bis 5 Runden lang in einen Elektrok\204fig.",  // THUNDERCAGE
  "Ein Blitz, der zuerst einschl\204gt. Schl\204gt fehl, wenn das Ziel nicht angreift.",  // THUNDERCLAP
  "Bei\341t mit elektrischen Z\204hnen zu. Ziel schreckt eventuell zur\201ck oder wird paralysiert.",  // THUNDERFANG
  "Ein elektrischer Schlag. Kann paralysieren.",  // THUNDERPUNCH
  "Ein Elektroangriff. Kann das Ziel paralysieren.",  // THUNDERSHOCK
  "Ein leichter Stromsto\341, der das Ziel paralysiert, wenn er trifft.",  // THUNDERWAVE
  "Bringt das Ziel zum Lachen und senkt Angriff und Verteidigung.",  // TICKLE
  "Erh\224ht Angriff und Initiative. Entfernt Effekte wie Stachler, Tarnsteine oder Delegator.",  // TIDYUP
  "St\224\341t gl\201hende Flammen aus, als w\201rde er singen. Erh\224ht den Sp.-Ang.",  // TORCHSONG
  "Qu\204lt das Ziel, sodass es keine Attacke 2-mal hintereinander einsetzen kann.",  // TORMENT
  "Vergiftet das Ziel schwer. Der Giftschaden steigt jede Runde.",  // TOXIC
  "Verteilt giftige Stacheln auf der Gegnerseite. Eingewechselte Pok\202mon werden vergiftet.",  // TOXICSPIKES
  "Greift mit giftigen F\204den an, die die Initiative senken und das Ziel vergiften.",  // TOXICTHREAD
  "Der Anwender verwandelt sich in eine Kopie des Ziels, mit denselben Attacken.",  // TRANSFORM
  "Ein Dreifachangriff. Kann paralysieren, verbrennen oder einfrieren.",  // TRIATTACK
  "\232berrumpelt das Ziel und tauscht die getragenen Items.",  // TRICK
  "Ein Dreifachsprung in perfektem Gleichtakt, der 3-mal hintereinander trifft.",  // TRIPLEDIVE
  "Trifft bis zu 3-mal hintereinander, jedes Mal st\204rker.",  // TRIPLEHIT
  "Tritt bis zu 3-mal hintereinander zu, jedes Mal st\204rker.",  // TRIPLEKICK
  "Ein Tritt mit tropischer Kraft, der den Angriff des Ziels senkt.",  // TROPKICK
  "Je weniger AP diese Attacke noch hat, desto st\204rker ist sie.",  // TRUMPCARD
  "Feuert 2 mysteri\224se Lichtstrahlen aus den Augen, die 2-mal hintereinander treffen.",  // TWINBEAM
  "Sticht das Ziel 2-mal mit Stacheln. Kann vergiften.",  // TWINEEDLE
  "Ein heftiger Tornado, der das Ziel zerfetzt. Ziel schreckt eventuell zur\201ck.",  // TWISTER
  "Macht 3 Runden lang Radau und h\204lt dabei alle wach.",  // UPROAR
  "Nach dem Angriff eilt der Anwender zur\201ck und tauscht mit einem Team-Pok\202mon.",  // UTURN
  "Dreht die F\204uste und feuert eine Vakuumwelle. Diese Attacke greift immer zuerst an.",  // VACUUMWAVE
  "Greift mit einer V-f\224rmigen Flamme an. Senkt Vert., Sp.-Vert. und Initiative des Anwenders.",  // VCREATE
  "Senkt Angriff, Sp.-Ang. und Initiative, aber nur bei bereits vergifteten Zielen.",  // VENOMDRENCH
  "\232bergie\341t das Ziel mit Gift. Ist es schon vergiftet, doppelter Schaden.",  // VENOSHOCK
  "Peitscht das Ziel mit d\201nnen Ranken.",  // VINEWHIP
  "Packt das Ziel mit kr\204ftigen Scheren.",  // VISEGRIP
  "Der Anwender greift als Letzter an, trifft aber immer.",  // VITALTHROW
  "Nach dem Angriff eilt der Anwender zur\201ck und tauscht mit einem Team-Pok\202mon.",  // VOLTSWITCH
  "Ein riskanter Rammangriff, der auch dem Anwender schadet.",  // VOLTTACKLE
  "Gro\341er Schaden gegen schlafende Ziele, weckt sie aber auch auf.",  // WAKEUPSLAP
  "Ein wuchtiger Sturmangriff. Ziel schreckt eventuell zur\201ck. Erklimmt auch Wasserf\204lle.",  // WATERFALL
  "Greift mit einem kr\204ftigen Wasserstrahl an.",  // WATERGUN
  "Ein starker Wasserstrahl. Kann das Ziel verwirren.",  // WATERPULSE
  "Trifft 2- bis 5-mal mit Wurfsternen. Diese Attacke greift immer zuerst an.",  // WATERSHURIKEN
  "Durchn\204sst sich selbst. Solange er im Kampf ist, werden Feuer-Attacken geschw\204cht.",  // WATERSPORT
  "Ein Wasserstrahl. Je weniger KP der Anwender hat, desto geringer der Schaden.",  // WATERSPOUT
  "H\201llt sich in Wasser und rammt das Ziel. Der Anwender erleidet schweren R\201cksto\341schaden.",  // WAVECRASH
  "Typ und St\204rke des Angriffs h\204ngen vom Wetter ab.",  // WEATHERBALL
  "Ein Strudel h\204lt das Ziel 4 bis 5 Runden lang fest.",  // WHIRLPOOL
  "Weht das Ziel weg und zwingt es zum Tausch. Gegen wilde Pok\202mon endet der Kampf.",  // WHIRLWIND
  "Ein vernichtender Schlag, der die finsteren K\201nste meisterhaft beherrscht. Immer ein Volltreffer.",  // WICKEDBLOW
  "Ruft ein Gewitter, das mit Wind und Blitzen angreift. Kann paralysieren.",  // WILDBOLTSTORM
  "Ein heftiger Elektro-Rammangriff. Der Anwender erleidet leichten R\201cksto\341schaden.",  // WILDCHARGE
  "Eine unheimliche violette Flamme, die das Ziel verbrennt.",  // WILLOWISP
  "Schl\204gt mit gro\341en, kr\204ftigen Fl\201geln zu.",  // WINGATTACK
  "Zieht sich in seinen harten Panzer zur\201ck und erh\224ht die Verteidigung.",  // WITHDRAW
  "Rammt den Gegner mit seinem robusten K\224rper. Der Anwender erleidet starken R\201cksto\341schaden.",  // WOODHAMMER
  "Erh\224ht Angriff und Spezial-Angriff des Anwenders.",  // WORKUP
  "Pflanzt dem Ziel einen Sorgensamen ein, der ihm die F\204higkeit Insomnia verleiht.",  // WORRYSEED
  "Umwickelt und quetscht das Ziel 4 bis 5 Runden lang mit Ranken oder dem K\224rper.",  // WRAP
  "Quetscht das Ziel kr\204ftig aus. Je mehr KP das Ziel hat, desto st\204rker der Angriff.",  // WRINGOUT
  "Kreuzt Sicheln oder Klauen wie eine Schere und greift an.",  // XSCISSOR
  "Ein gro\341es G\204hnen, das das Ziel in der n\204chsten Runde einschlafen l\204sst.",  // YAWN
  "Feuert eine elektrische Ladung, die Schaden zuf\201gt und paralysiert.",  // ZAPCANNON
  "B\201ndelt Psycho-Energie im Kopf und greift an. Ziel schreckt eventuell zur\201ck.",  // ZENHEADBUTT
  "St\201rzt sich auf das Ziel und setzt eine starke Entladung frei. Ziel schreckt eventuell zur\201ck.",  // ZINGZAP
  "Das Ziel kann sein getragenes Item nicht nutzen, und sein Trainer kann keine Items bei ihm einsetzen.",  // EMBARGO
  "Das Ziel kann 5 Runden lang keine Attacken, F\204higkeiten oder Items nutzen, die KP auff\201llen.",  // HEALBLOCK
  "Wer diesen Gesang h\224rt und nicht innerhalb von 3 Runden ausgewechselt wird, wird besiegt.",  // PERISHSONG
  "Opfert eigene KP, um einen Delegator zu erzeugen, der Treffer abf\204ngt.",  // SUBSTITUTE
  "Erzeugt 5 Runden lang einen seltsamen Raum, in dem langsame Pok\202mon zuerst handeln.",  // TRICKROOM
  "Ein Wunsch, der in der n\204chsten Runde die H\204lfte der max. KP auff\201llt.",  // WISH
};

static const char *const MOVE_DESC_IT[MOVE_COUNT] = {
  "Si usa quando non restano mosse. Ferisce anche chi la usa.",  // STRUGGLE
  "Assorbe met\205 dei danni inflitti.",  // ABSORB
  "Si scaglia contro il bersaglio ad alta velocit\205. Colpisce sempre per primo.",  // ACCELEROCK
  "Spruzza un acido corrosivo. Pu\225 ridurre la Difesa Speciale.",  // ACID
  "Modifica la sua struttura cellulare liquefacendosi e aumenta molto la Difesa.",  // ACIDARMOR
  "Attacca con un liquido corrosivo che riduce molto la Difesa Speciale del bersaglio.",  // ACIDSPRAY
  "Colpisce agilmente. Se chi la usa non tiene strumenti, infligge danni ingenti.",  // ACROBATICS
  "Preme punti chiave del corpo e aumenta molto una statistica a caso.",  // ACUPRESSURE
  "Sferra un attacco rapidissimo che non si pu\225 evitare.",  // AERIALACE
  "Spara un getto d'aria con alta probabilit\205 di brutto colpo.",  // AEROBLAST
  "Rilassa il corpo per aumentare molto la Velocit\205.",  // AGILITY
  "Vento tagliente che sferza. Alta probabilit\205 di brutto colpo.",  // AIRCUTTER
  "Attacca con una lama d'aria che fende persino il cielo. Pu\225 far tentennare il bersaglio.",  // AIRSLASH
  "Dimentica le sue preoccupazioni e aumenta molto la Difesa Speciale.",  // AMNESIA
  "Lancia un'ancora contro il bersaglio, che resta intrappolato e non pu\225 fuggire.",  // ANCHORSHOT
  "Attacco preistorico che pu\225 aumentare tutte le statistiche di chi lo usa.",  // ANCIENTPOWER
  "Attacca con il succo corrosivo di una mela acerba e riduce la Difesa Speciale.",  // APPLEACID
  "Taglia il bersaglio con acqua ad alta pressione come una lama. Alta probabilit\205 di brutto colpo.",  // AQUACUTTER
  "Attacco rapido che permette di colpire per primo.",  // AQUAJET
  "Un velo d'acqua avvolge chi lo usa, che recupera un po' di PS a ogni turno.",  // AQUARING
  "Esegue una danza elegante e fluida che infligge danni. Aumenta la Velocit\205 di chi la usa.",  // AQUASTEP
  "Agita la coda come un'onda furiosa in una tempesta devastante.",  // AQUATAIL
  "Lancia parti della sua armatura come proiettili ardenti. Riduce Difesa e Dif. Sp. di chi la usa.",  // ARMORCANNON
  "Spintoni diretti che colpiscono da 2 a 5 volte di fila.",  // ARMTHRUST
  "Cura i problemi di stato della squadra con un aroma delicato.",  // AROMATHERAPY
  "Aumenta la Difesa Speciale di un Pok\202mon della squadra con una fragranza misteriosa.",  // AROMATICMIST
  "Se il bersaglio ha gi\205 subito danni in questo turno, la potenza raddoppia.",  // ASSURANCE
  "Impressiona il bersaglio e pu\225 farlo tentennare.",  // ASTONISH
  "Morde con fauci particolarmente efficaci contro i Pok\202mon di tipo Coleottero.",  // ATRAPAMOSCAS
  "Chiama i suoi compagni ad attaccare il bersaglio. Alta probabilit\205 di brutto colpo.",  // ATTACKORDER
  "Se il bersaglio \212 del sesso opposto, si innamora e attacca con meno probabilit\205.",  // ATTRACT
  "Libera dal suo interno una scarica di forza aura. Non manca mai il bersaglio.",  // AURASPHERE
  "Attacca e aumenta la Velocit\205 di chi la usa. Il tipo cambia in base alla forma di chi la usa.",  // AURAWHEEL
  "Raggio multicolore che pu\225 ridurre l'Attacco.",  // AURORABEAM
  "Si scrolla di dosso del peso per alleggerirsi e aumenta molto la Velocit\205.",  // AUTOTOMIZE
  "Infligge danni doppi a un bersaglio che ha colpito chi la usa nello stesso turno.",  // AVALANCHE
  "Calcia in aria e colpisce col tallone. Se fallisce, ferisce chi la usa. Pu\225 confondere.",  // AXEKICK
  "Guarda il bersaglio con occhioni dolci e ne riduce l'Attacco. Colpisce sempre per primo.",  // BABYDOLLEYES
  "Fa il cattivo, attacca il bersaglio e crea una barriera Riflesso.",  // BADDYBAD
  "Protegge dagli attacchi e avvelena chi lo colpisce con una mossa di contatto.",  // BANEFULBUNKER
  "Spara aculei tossici che possono avvelenare. Potenza doppia se il bersaglio \212 avvelenato.",  // BARBBARRAGE
  "Scaglia sfere contro il bersaglio da 2 a 5 volte di fila.",  // BARRAGE
  "Crea una barriera che aumenta molto la Difesa.",  // BARRIER
  "Scambia il posto con un compagno e gli passa le modifiche alle statistiche.",  // BATONPASS
  "Scalda il becco prima di attaccare. Scotta chi lo colpisce con un contatto durante la carica.",  // BEAKBLAST
  "Attacco di tutta la squadra. Pi\227 compagni ci sono, pi\227 colpi infligge.",  // BEATUP
  "Infligge danni con un rutto. Per usarla deve aver mangiato una Bacca tenuta.",  // BELCH
  "Dimezza i propri PS massimi per portare l'Attacco al massimo.",  // BELLYDRUM
  "Resiste agli attacchi per 2 turni, poi libera l'energia accumulata con danni doppi.",  // BIDE
  "Lega e stritola il bersaglio da 4 a 5 turni.",  // BIND
  "Un morso vorace che pu\225 far tentennare il bersaglio.",  // BITE
  "Sferra una stoccata piena di rancore. Recupera PS pari a met\205 dei danni inflitti.",  // BITTERBLADE
  "Attacca il bersaglio con un freddo rancore e ne riduce l'Attacco.",  // BITTERMALICE
  "Calcio con alta probabilit\205 di brutto colpo. Pu\225 scottare.",  // BLAZEKICK
  "Vento gelido che fa rabbrividire e pu\225 ridurre la Velocit\205 del bersaglio.",  // BLEAKWINDSTORM
  "Tempesta di ghiaccio che pu\225 congelare.",  // BLIZZARD
  "Sbarra la strada al bersaglio impedendogli di fuggire.",  // BLOCK
  "Avvolge il bersaglio in una fiamma blu bella ma potente. Pu\225 scottarlo.",  // BLUEFLARE
  "Carica il bersaglio. Pi\227 \212 alta la Difesa di chi la usa, pi\227 danni infligge.",  // BODYPRESS
  "Si lancia sul bersaglio con tutto il suo peso. Pu\225 paralizzare.",  // BODYSLAM
  "Attacca col becco elettrificato. Se agisce prima del bersaglio, la potenza raddoppia.",  // BOLTBEAK
  "Si avvolge di una forte carica elettrica e si scaglia sul bersaglio. Pu\225 paralizzare.",  // BOLTSTRIKE
  "Colpisce con un osso. Pu\225 far tentennare il bersaglio.",  // BONECLUB
  "Lancia un osso come un boomerang e colpisce 2 volte.",  // BONEMERANG
  "Osso in pugno, colpisce il bersaglio da 2 a 5 volte.",  // BONERUSH
  "Attacca tutti i Pok\202mon intorno con un'onda sonora potentissima.",  // BOOMBURST
  "1\370 turno: rimbalza in alto. 2\370 turno: colpisce. Pu\225 paralizzare.",  // BOUNCE
  "Spara bolle d'acqua al bersaglio e assorbe met\205 dei danni inflitti.",  // BOUNCYBUBBLE
  "Attacca pungendo il bersaglio con un ramo appuntito.",  // BRANCHPOKE
  "Ripiega le ali e attacca in volo radente. Anche chi la usa subisce danni.",  // BRAVEBIRD
  "Sferza con foga il bersaglio con la coda e ne riduce l'Attacco.",  // BREAKINGSWIPE
  "Attacco potente che distrugge anche barriere come Schermoluce e Riflesso.",  // BRICKBREAK
  "Se il bersaglio ha met\205 dei PS o meno, l'attacco \212 due volte pi\227 forte.",  // BRINE
  "Fa roteare il corpo per infliggere danni a tutti i nemici.",  // BRUTALSWING
  "Spruzza bolle contro il nemico. Pu\225 ridurre la Velocit\205.",  // BUBBLE
  "Diluvio di bolle che pu\225 ridurre la Velocit\205.",  // BUBBLEBEAM
  "Morde il nemico. Se tiene una Bacca, chi la usa la mangia e ne ottiene l'effetto.",  // BUGBITE
  "Il battito delle ali crea un'onda sonora dannosa. Pu\225 ridurre la Difesa Speciale del bersaglio.",  // BUGBUZZ
  "Irrobustisce il corpo per aumentare Attacco e Difesa.",  // BULKUP
  "Scossa sismica che colpisce i Pok\202mon adiacenti e ne riduce la Velocit\205.",  // BULLDOZE
  "Pugno potente e veloce come un proiettile. Colpisce sempre per primo.",  // BULLETPUNCH
  "Spara rapidamente da 2 a 5 raffiche di semi di fila.",  // BULLETSEED
  "Usa il pelo per proteggersi dagli attacchi e scotta chi lo tocca.",  // BURNINGBULWARK
  "Usa fino all'ultima fiamma del suo corpo per attaccare, ma perde il tipo Fuoco.",  // BURNUP
  "Spara una scarica elettrica contro il bersaglio. Paralizza sempre.",  // BUZZYBUZZ
  "Si concentra e calma lo spirito per aumentare Attacco Speciale e Difesa Speciale.",  // CALMMIND
  "Se il bersaglio \212 del sesso opposto, resta ammaliato e il suo Attacco Speciale cala molto.",  // CAPTIVATE
  "Accumula energia per potenziare la prossima mossa di tipo Elettro. Aumenta la Difesa Speciale.",  // CHARGE
  "Spara un raggio elettrico al nemico. Pu\225 aumentare l'Attacco Speciale di chi la usa.",  // CHARGEBEAM
  "Ammalia il bersaglio e ne riduce molto l'Attacco.",  // CHARM
  "Attacca con un'onda sonora fatta di parole imparate. Pu\225 confondere il bersaglio.",  // CHATTER
  "Spruzza sul bersaglio acqua gelida e scoraggiante che ne riduce l'Attacco.",  // CHILLINGWATER
  "Racconta una battuta cos\215 fredda da far nevicare e si scambia con un Pok\202mon della squadra.",  // CHILLYRECEPTION
  "Colpisce i punti deboli del bersaglio ignorando le modifiche alle statistiche.",  // CHIPAWAY
  "Concentra la clorofilla e la spara come un raggio, ma ferisce anche se stesso.",  // CHLOROBLAST
  "Lancia in aria il bersaglio e lo costringe a cambiare. Contro un Pok\202mon selvatico, la lotta finisce.",  // CIRCLETHROW
  "Afferra e stringe forte il bersaglio per 4 o 5 turni.",  // CLAMP
  "Sfrega le squame producendo un forte suono, ma riduce la propria Difesa.",  // CLANGINGSCALES
  "Aumenta tutte le statistiche di chi la usa a costo di parte dei PS.",  // CLANGOROUSSOUL
  "Attacca con una strana nebbia che annulla le modifiche alle statistiche del bersaglio.",  // CLEARSMOG
  "Lotta a viso aperto contro il nemico. Riduce Difesa e Difesa Speciale di chi la usa.",  // CLOSECOMBAT
  "Si concentra per aumentare Attacco, Difesa e Precisione.",  // COIL
  "Si schianta al suolo mentre si trasforma. La potenza aumenta se \212 superefficace.",  // COLLISIONCOURSE
  "Colpisce da 2 a 5 volte di fila.",  // COMETPUNCH
  "Restituisce al nemico l'ultimo attacco subito, ma con molta pi\227 forza.",  // COMEUPPANCE
  "Distrae il nemico confidandogli un segreto e ne riduce l'Attacco Speciale.",  // CONFIDE
  "Raggio sinistro che confonde il bersaglio.",  // CONFUSERAY
  "Debole attacco telecinetico che pu\225 confondere.",  // CONFUSION
  "Stringe per ferire e pu\225 ridurre la Velocit\205.",  // CONSTRICT
  "Imita la mossa usata subito prima. Fallisce se non ne \212 ancora stata usata nessuna.",  // COPYCAT
  "Danneggia il nemico e, se ha gi\205 usato una mossa, ne annulla l'abilit\205.",  // COREENFORCER
  "Aumenta Difesa e Difesa Speciale con un'energia mistica.",  // COSMICPOWER
  "Si avvolge in un soffice batuffolo protettivo. Aumenta moltissimo la Difesa.",  // COTTONGUARD
  "Attacca spore al bersaglio e ne riduce molto la Velocit\205.",  // COTTONSPORE
  "Restituisce un colpo fisico con danni doppi.",  // COUNTER
  "Si avvicina con tenerezza al bersaglio, ma lo attacca e pu\225 rubargli lo strumento.",  // COVET
  "Colpisce con forza con le chele. Alta probabilit\205 di brutto colpo.",  // CRABHAMMER
  "Doppio colpo di taglio con alta probabilit\205 di brutto colpo.",  // CROSSCHOP
  "Fendente che pu\225 avvelenare il bersaglio. Alta probabilit\205 di brutto colpo.",  // CROSSPOISON
  "Mastica con zanne affilate e pu\225 ridurre la Difesa del bersaglio.",  // CRUNCH
  "Fa a pezzi il bersaglio con artigli affilati e pu\225 ridurne la Difesa.",  // CRUSHCLAW
  "Pi\227 PS restano al bersaglio, pi\227 l'attacco \212 forte.",  // CRUSHGRIP
  "Mossa con effetti diversi a seconda che chi la usa sia di tipo Spettro o no.",  // CURSE
  "Taglia con artigli, falci, ecc. Serve anche a tagliare alberelli.",  // CUT
  "Ruota e colpisce il nemico ignorando le modifiche alle statistiche del bersaglio.",  // DARKESTLARIAT
  "Libera un'aura orribile piena di pensieri malvagi. Pu\225 far tentennare il bersaglio.",  // DARKPULSE
  "Trascina il bersaglio in un mondo di tenebre che lo fa addormentare.",  // DARKVOID
  "Danneggia i nemici con una luce potente.",  // DAZZLINGGLEAM
  "Chiama i suoi compagni a formare uno scudo vivente. Aumenta Difesa e Dif. Sp.",  // DEFENDORDER
  "Si arrotola per nascondere i punti deboli. Aumenta la Difesa.",  // DEFENSECURL
  "Vento potente che spazza via Riflesso o Schermoluce del bersaglio. Pu\225 ridurne l'Elusione.",  // DEFOG
  "Scivola con uno sci e aumenta la Velocit\205 di chi la usa.",  // DESLIZAMIENTO
  "Se chi la usa va KO, anche il nemico va KO.",  // DESTINYBOND
  "Blocca gli attacchi, ma pu\225 fallire se usata pi\227 volte di fila.",  // DETECT
  "Bufera devastante di diamanti. Pu\225 aumentare la Difesa di chi la usa.",  // DIAMONDSTORM
  "1\370 turno: scava. 2\370 turno: attacca. Serve anche per uscire da certe zone.",  // DIG
  "Colpisce il punto debole con artigli letali. Pu\225 avvelenare, paralizzare o far addormentare.",  // DIRECLAW
  "Blocca l'ultima mossa usata dal bersaglio per 4 turni.",  // DISABLE
  "Ammalia i nemici con la voce infliggendo danni emotivi. Non manca mai il bersaglio.",  // DISARMINGVOICE
  "Un'abbagliante onda elettrica colpisce gli altri Pok\202mon in campo. Pu\225 paralizzare.",  // DISCHARGE
  "1\370 turno: si immerge in mare. 2\370 turno: riemerge e attacca.",  // DIVE
  "Pugni ritmici che possono confondere.",  // DIZZYPUNCH
  "Concentra un fascio di luce e attacca 2 turni dopo.",  // DOOMDESIRE
  "Attacco spericolato che ferisce anche chi lo usa.",  // DOUBLEEDGE
  "Colpisce il bersaglio 2 volte di fila con la coda o altre parti del corpo.",  // DOUBLEHIT
  "Colpisce 2 volte di fila con i pugni. Pu\225 far tentennare il nemico.",  // DOUBLEIRONBASH
  "Un doppio calcio che colpisce 2 volte.",  // DOUBLEKICK
  "Libera tutta l'elettricit\205 del corpo per attaccare. Perde il tipo Elettro.",  // DOUBLESHOCK
  "Schiaffeggia da 2 a 5 volte di fila.",  // DOUBLESLAP
  "Crea copie di s\202 per aumentare l'Elusione.",  // DOUBLETEAM
  "Fa cadere grandi comete dal cielo. Riduce molto l'Attacco Speciale di chi la usa.",  // DRACOMETEOR
  "Piomba dal cielo ad alta velocit\205, ma riduce Difesa e Dif. Sp. di chi la usa.",  // DRAGONASCENT
  "Potente soffio che colpisce il bersaglio e pu\225 paralizzarlo.",  // DRAGONBREATH
  "Graffia il bersaglio con artigli affilati.",  // DRAGONCLAW
  "Danza mistica che aumenta Attacco e Velocit\205.",  // DRAGONDANCE
  "Attacca 2 volte usando Dreepy. Se ci sono 2 nemici, li colpisce una volta ciascuno.",  // DRAGONDARTS
  "Attacca con la forza vitale. Meno PS ha chi la usa, meno danni infligge.",  // DRAGONENERGY
  "Usa il corpo come un martello e si scaglia sul nemico per danneggiarlo.",  // DRAGONHAMMER
  "Spalanca la bocca e libera un'onda d'urto che colpisce il bersaglio.",  // DRAGONPULSE
  "Onda d'urto furiosa che toglie sempre 40 PS.",  // DRAGONRAGE
  "Attacca brutalmente intimidendo il bersaglio. Pu\225 farlo tentennare.",  // DRAGONRUSH
  "Colpisce il bersaglio e lo costringe a cambiare. Contro un Pok\202mon selvatico, la lotta finisce.",  // DRAGONTAIL
  "Assorbe PS dal bersaglio con un bacio e ripristina i propri.",  // DRAININGKISS
  "Pugno che prosciuga energia. Recupera met\205 dei PS tolti al bersaglio.",  // DRAINPUNCH
  "Recupera met\205 dei danni inflitti a un bersaglio addormentato.",  // DREAMEATER
  "Beccata rotante e perforante molto potente.",  // DRILLPECK
  "Colpisce usando il corpo come un trapano. Alta probabilit\205 di brutto colpo.",  // DRILLRUN
  "Controlla un ceppo percuotendolo e attacca riducendo la Velocit\205 del bersaglio.",  // DRUMBEATING
  "Colpisce 2 volte di fila con la coda o altre parti del corpo.",  // DUALCHOP
  "Colpisce il nemico 2 volte con le ali.",  // DUALWINGBEAT
  "Spara un potente raggio di energia dal suo nucleo.",  // DYNAMAXCANNON
  "Pugno con tutta la forza concentrata. Se va a segno, confonde.",  // DYNAMICPUNCH
  "Fa eruttare il terreno sotto il nemico. Pu\225 ridurre la Difesa Speciale.",  // EARTHPOWER
  "Un terremoto che colpisce tutti i Pok\202mon intorno.",  // EARTHQUAKE
  "Un sussurro che diventa pi\227 potente man mano che chi la usa e altri Pok\202mon la usano.",  // ECHOEDVOICE
  "Emana strane onde che riducono molto l'Attacco Speciale del bersaglio.",  // EERIEIMPULSE
  "Scaglia con forza un uovo contro il bersaglio.",  // EGGBOMB
  "Crea un campo elettrico per 5 turni che impedisce ai Pok\202mon a terra di addormentarsi.",  // ELECTRICTERRAIN
  "Lancia una sfera elettrica. Pi\227 \212 alta la Velocit\205 di chi la usa, pi\227 danni infligge.",  // ELECTROBALL
  "Balza trasformandosi e trafigge il nemico. La potenza aumenta se \212 superefficace.",  // ELECTRODRIFT
  "1\370 turno: accumula elettricit\205 e aumenta l'Att. Sp. 2\370 turno: attacca. Con la pioggia attacca al 1\370 turno.",  // ELECTROSHOT
  "Intrappola e attacca i bersagli con una ragnatela elettrica. Ne riduce la Velocit\205.",  // ELECTROWEB
  "Attacca con piccole fiamme che possono scottare.",  // EMBER
  "Costringe il bersaglio a ripetere la sua ultima mossa per 3 turni.",  // ENCORE
  "Riduce i PS del bersaglio fino a pareggiarli con quelli di chi la usa.",  // ENDEAVOR
  "Resiste a qualunque attacco restando con almeno 1 PS. Pu\225 fallire se usata pi\227 volte di fila.",  // ENDURE
  "Raccoglie le forze della natura e le libera. Pu\225 ridurre la Difesa Speciale del bersaglio.",  // ENERGYBALL
  "Una strana danza che d\205 a chi la usa e al bersaglio la stessa abilit\205.",  // ENTRAINMENT
  "Furia esplosiva. Meno PS ha chi la usa, meno danni infligge.",  // ERUPTION
  "Provoca un terribile brivido al bersaglio e lo congela.",  // ESCALOFRIO
  "Taglia con ali cariche di aura. Alta probabilit\205 di brutto colpo. Aumenta la Velocit\205.",  // ESPERWING
  "L'attacco pi\227 potente di Eternatus nella sua forma originale. Al turno dopo non pu\225 muoversi.",  // ETERNABEAM
  "Attacca con poteri psichici. Sul Campo Psichico la potenza aumenta e colpisce tutti i nemici.",  // EXPANDINGFORCE
  "Esplode e ferisce tutti i Pok\202mon intorno. Chi la usa va subito KO.",  // EXPLOSION
  "Energia stranissima che pu\225 far tentennare il bersaglio.",  // EXTRASENSORY
  "Attacco rapidissimo che colpisce sempre per primo.",  // EXTREMESPEED
  "La potenza raddoppia se chi la usa \212 scottato, paralizzato o avvelenato.",  // FACADE
  "Scatena un vento fatato che sferza il bersaglio.",  // FAIRYWIND
  "Colpisce per primo e fa tentennare il nemico. Funziona solo al primo turno.",  // FAKEOUT
  "Lacrime di coccodrillo che riducono molto la Difesa Speciale del bersaglio.",  // FAKETEARS
  "Finge un inchino e trafigge il bersaglio con i capelli arruffati. Non manca mai.",  // FALSESURRENDER
  "Attacco trattenuto che lascia al bersaglio almeno 1 PS.",  // FALSESWIPE
  "Avvolge il bersaglio in un manto di piume e ne riduce molto l'Attacco.",  // FEATHERDANCE
  "Colpisce i bersagli che usano Protezione o Individua e annulla tali mosse.",  // FEINT
  "Inganna il bersaglio per avvicinarsi e sferra un pugno che non manca mai.",  // FEINTATTACK
  "Se manda KO il bersaglio con questa mossa, l'Attacco di chi la usa aumenta moltissimo.",  // FELLSTINGER
  "A volte le altre teste si uniscono all'attacco e infligge danni doppi.",  // FICKLEBEAM
  "Colpisce avvolto dalle fiamme. Pu\225 aumentare l'Attacco Speciale di chi la usa.",  // FIERYDANCE
  "Si sacrifica e infligge al bersaglio danni pari ai PS che perde.",  // FINALGAMBIT
  "Fiammata intensa che brucia il bersaglio e pu\225 scottarlo.",  // FIREBLAST
  "Morde con zanne infuocate. Pu\225 far tentennare o scottare il bersaglio.",  // FIREFANG
  "Colpisce il nemico con una frusta incandescente e ne riduce la Difesa.",  // FIRELASH
  "Pugno infuocato. Pu\225 scottare.",  // FIREPUNCH
  "Un anello di fuoco intrappola il bersaglio da 4 a 5 turni.",  // FIRESPIN
  "Mossa potente e con priorit\205, usabile solo al primo turno di chi la usa.",  // FIRSTIMPRESSION
  "Attacca con le branchie. Se agisce prima del bersaglio, la potenza raddoppia.",  // FISHIOUSREND
  "Apre una crepa nel terreno e ci fa cadere il bersaglio. Lo manda subito KO.",  // FISSURE
  "Attacco frenetico. Meno PS ha chi la usa, pi\227 danni infligge.",  // FLAIL
  "Colpisce con una fiammata che danneggia anche i Pok\202mon adiacenti al bersaglio.",  // FLAMEBURST
  "Fiamme che colpiscono il bersaglio e aumentano la Velocit\205 di chi la usa.",  // FLAMECHARGE
  "Attacca con una grande fiammata che pu\225 scottare.",  // FLAMETHROWER
  "Attacca avvolto dal fuoco. Pu\225 scottare.",  // FLAMEWHEEL
  "Si avvolge di fiamme e carica il bersaglio, ma subisce danni anche lui.",  // FLAREBLITZ
  "Luce accecante che riduce la Precisione del bersaglio.",  // FLASH
  "Concentra tutta la luce del corpo e la libera. Pu\225 ridurre la Difesa Speciale del bersaglio.",  // FLASHCANNON
  "Lusinga il bersaglio e lo confonde, ma ne aumenta l'Attacco Speciale.",  // FLATTER
  "Emette un potente raggio, ma l'Attacco Speciale di chi la usa cala molto.",  // FLEURCANNON
  "Lancia lo strumento che tiene. Potenza ed effetto variano in base allo strumento.",  // FLING
  "Dopo l'attacco, chi la usa lascia in fretta il posto a un altro Pok\202mon della squadra.",  // FLIPTURN
  "Lancia un mazzo di fiori truccato. Non manca mai e infligge sempre un brutto colpo.",  // FLOWERTRICK
  "1\370 turno: si alza in volo. 2\370 turno: attacca il bersaglio.",  // FLY
  "Si lancia sul nemico. Questa mossa \212 di tipo Lotta e Volante allo stesso tempo.",  // FLYINGPRESS
  "Affina la concentrazione e libera il suo potere. Pu\225 ridurre la Dif. Sp. del nemico.",  // FOCUSBLAST
  "Concentra energia per aumentare la probabilit\205 di brutto colpo.",  // FOCUSENERGY
  "Si concentra per sferrare un pugno. Fallisce se viene colpito prima di usarla.",  // FOCUSPUNCH
  "Colpisce il bersaglio con un'onda d'urto. Pu\225 paralizzarlo.",  // FORCEPALM
  "Permette di colpire i Pok\202mon di tipo Spettro con qualsiasi mossa e quelli elusivi.",  // FORESIGHT
  "Usa l'Attacco del bersaglio per colpire.",  // FOULPLAY
  "Raffredda di colpo il bersaglio e pu\225 congelarlo. Superefficace sui Pok\202mon di tipo Acqua.",  // FREEZEDRY
  "Attacca con un cristallo di nebbia gelata. Annulla le modifiche alle statistiche di tutti.",  // FREEZYFROST
  "Attacco di aria gelida. Infligge sempre un brutto colpo.",  // FROSTBREATH
  "Pi\227 \212 bassa l'amicizia con l'Allenatore, pi\227 questo attacco \212 potente.",  // FRUSTRATION
  "Colpisce il bersaglio con corna o becco da 2 a 5 volte.",  // FURYATTACK
  "Attacco con artigli o falci che si intensifica se usato pi\227 volte di fila.",  // FURYCUTTER
  "Graffia rapidamente da 2 a 5 volte.",  // FURYSWIPES
  "Enorme scarica elettrica. La potenza aumenta se influenzata da un'enorme fiammata.",  // FUSIONBOLT
  "Fiammata gigantesca. La potenza aumenta se influenzata da una grande energia elettrica.",  // FUSIONFLARE
  "Concentra energia psichica per colpire il bersaglio 2 turni dopo.",  // FUTURESIGHT
  "Sputa succhi gastrici sul bersaglio e ne annulla l'abilit\205.",  // GASTROACID
  "Fa roteare due ingranaggi d'acciaio sul bersaglio. Colpisce 2 volte.",  // GEARGRIND
  "1\370 turno: concentra energia. 2\370 turno: aumenta molto Att. Sp., Dif. Sp. e Velocit\205.",  // GEOMANCY
  "Assorbe met\205 dei danni inflitti.",  // GIGADRAIN
  "Carica il nemico con tutta la sua forza. Al turno dopo deve riposare.",  // GIGAIMPACT
  "Colpisce con un enorme martello. Non si pu\225 usare 2 volte di fila.",  // GIGATONHAMMER
  "Attacco di aria gelida che pu\225 ridurre la Velocit\205 del bersaglio.",  // GLACIATE
  "Al turno successivo gli attacchi del nemico non falliscono e infliggono danni doppi.",  // GLAIVERUSH
  "Intimidisce il bersaglio con lo sguardo e lo paralizza.",  // GLARE
  "Bombarda il bersaglio con forza telecinetica e crea uno Schermoluce.",  // GLITZYGLOW
  "Avvinghia il bersaglio con l'erba e lo fa cadere. Pi\227 pesa il bersaglio, pi\227 danni infligge.",  // GRASSKNOT
  "Melodia piacevole che fa addormentare il bersaglio.",  // GRASSWHISTLE
  "Attacca scivolando sul terreno. Sul Campo Erboso ha la priorit\205.",  // GRASSYGLIDE
  "Crea un Campo Erboso per 5 turni. A fine turno i Pok\202mon a terra recuperano PS.",  // GRASSYTERRAIN
  "Infligge danni facendo cadere una mela da grande altezza. Riduce la Difesa del bersaglio.",  // GRAVAPPLE
  "Ruggito tenero che riduce l'Attacco del nemico.",  // GROWL
  "Il corpo cresce rapidamente e aumentano Attacco e Attacco Speciale.",  // GROWTH
  "Se chi la usa va KO per un attacco, i PP di quell'attacco si azzerano.",  // GRUDGE
  "Attacco con le chele che manda KO il nemico in un colpo, se va a segno.",  // GUILLOTINE
  "Scaglia rifiuti disgustosi contro il bersaglio. Pu\225 avvelenarlo.",  // GUNKSHOT
  "Crea un tornado con le ali e lo scaglia contro il bersaglio.",  // GUST
  "Carica con un potente attacco rotante. Pi\227 \212 lento chi la usa, pi\227 danni infligge.",  // GYROBALL
  "Ruota il pugno e sferra un colpo pesante. Riduce per\225 la Velocit\205 di chi la usa.",  // HAMMERARM
  "Tende i muscoli per aumentare la Difesa.",  // HARDEN
  "Nebbia che annulla le modifiche alle statistiche di tutti i Pok\202mon in lotta.",  // HAZE
  "Sferra una potente testata che pu\225 far tentennare il bersaglio.",  // HEADBUTT
  "Sferra una tremenda testata. Ferisce un po' anche chi la usa.",  // HEADCHARGE
  "Carica con tutte le sue forze, ma riduce Difesa e Difesa Speciale di chi la usa.",  // HEADLONGRUSH
  "Rischia la vita con una testata a piena forza. Chi la usa subisce gravi danni.",  // HEADSMASH
  "Un rintocco che cura i problemi di stato di tutti i Pok\202mon della squadra.",  // HEALBELL
  "Chi la usa va KO, ma il Pok\202mon che lo sostituisce recupera PS e stato.",  // HEALINGWISH
  "Chiama i suoi compagni a curarlo. Recupera fino a met\205 dei PS massimi.",  // HEALORDER
  "Un'onda curativa ripristina met\205 dei PS massimi del bersaglio.",  // HEALPULSE
  "Un gesto aggraziato che sferra un colpo tremendo. Pu\225 far tentennare il bersaglio.",  // HEARTSTAMP
  "Pi\227 chi la usa pesa rispetto al bersaglio, pi\227 danni infligge.",  // HEATCRASH
  "Vento rovente che pu\225 scottare il bersaglio.",  // HEATWAVE
  "Pi\227 chi la usa pesa rispetto al bersaglio, pi\227 danni infligge.",  // HEAVYSLAM
  "Attacco che infligge danni ingenti ai bersagli con problemi di stato.",  // HEX
  "Attacco unico il cui tipo e la cui potenza variano in base a chi lo usa.",  // HIDDENPOWER
  "Sferra un colpo devastante usando tutto il corpo.",  // HIGHHORSEPOWER
  "Salta in alto e sferra un calcio. Se fallisce, ferisce chi la usa.",  // HIGHJUMPKICK
  "Si affila gli artigli per aumentare Attacco e Precisione.",  // HONECLAWS
  "Colpisce il bersaglio con una cornata.",  // HORNATTACK
  "Attacco a trapano che manda KO il bersaglio in un colpo, se va a segno.",  // HORNDRILL
  "Colpo che prosciuga energia. Recupera met\205 dei PS tolti al bersaglio.",  // HORNLEECH
  "Ululato che d\205 la carica e aumenta l'Attacco.",  // HOWL
  "Colpisce con un forte vortice che avvolge il nemico. Pu\225 confonderlo.",  // HURRICANE
  "Spara un'enorme massa d'acqua ad alta pressione.",  // HYDROPUMP
  "Spara un potente getto d'acqua bollente. La potenza aumenta col sole intenso.",  // HYDROSTEAM
  "Un raggio devastante, ma al turno dopo chi lo usa deve riposare.",  // HYPERBEAM
  "Attacca facendo ruotare la parte appuntita del corpo. Ignora mosse come Protezione.",  // HYPERDRILL
  "Attacco con zanne affilate. Pu\225 far tentennare.",  // HYPERFANG
  "Colpisce anche se il nemico usa Protezione, Individua, Ampiaguardia o Anticipo nel turno.",  // HYPERSPACEHOLE
  "Urlo lancinante che infligge danni al bersaglio.",  // HYPERVOICE
  "Ipnosi che fa addormentare profondamente il bersaglio.",  // HYPNOSIS
  "Rotola contro il bersaglio per 5 turni, ogni volta con pi\227 forza.",  // ICEBALL
  "Raggio di ghiaccio che pu\225 congelare.",  // ICEBEAM
  "Morde con zanne gelate. Pu\225 far tentennare o congelare il bersaglio.",  // ICEFANG
  "Un pugno tremendo colpisce il nemico, ma la Velocit\205 di chi la usa cala.",  // ICEHAMMER
  "Pugno gelido. Pu\225 congelare.",  // ICEPUNCH
  "Crea blocchi di ghiaccio e li scaglia a gran velocit\205. Colpisce sempre per primo.",  // ICESHARD
  "Ricopre gli arti di ghiaccio e colpisce il nemico. Se c'\212 un campo, lo distrugge.",  // ICESPINNER
  "Scaglia grandi ghiaccioli. Pu\225 far tentennare il bersaglio.",  // ICICLECRASH
  "Scaglia da 2 a 5 raffiche di ghiaccioli di fila.",  // ICICLESPEAR
  "Attacco di aria gelida che riduce la Velocit\205 del bersaglio.",  // ICYWIND
  "Impedisce ai nemici di usare le mosse che conosce anche chi la usa.",  // IMPRISON
  "Fiamme che colpiscono i bersagli adiacenti. Le Bacche che tengono bruciano e spariscono.",  // INCINERATE
  "Attacca con una grande fiammata che scotta il bersaglio.",  // INFERNO
  "Stringe il nemico da 4 a 5 turni, durante i quali non pu\225 fuggire.",  // INFESTATION
  "Mette radici per recuperare PS a ogni turno, ma non pu\225 cambiare.",  // INGRAIN
  "Diffonde particelle cariche che rendono di tipo Elettro le mosse di tipo Normale.",  // IONDELUGE
  "Indurisce il corpo come il ferro e aumenta molto la Difesa.",  // IRONDEFENSE
  "Colpisce con la sua dura testa d'acciaio. Pu\225 far tentennare il bersaglio.",  // IRONHEAD
  "Colpisce con una coda d'acciaio. Pu\225 ridurre la Difesa del bersaglio.",  // IRONTAIL
  "Colpisce con una clava. Il tipo varia in base alla maschera. Alta probabilit\205 di brutto colpo.",  // IVYCUDGEL
  "Impedisce a chi la usa e al bersaglio di lasciare la lotta finch\202 uno dei due non va KO.",  // JAWLOCK
  "Avvolge il pugno in un torrente d'acqua e sferra un colpo con priorit\205.",  // JETPUNCH
  "Emette innumerevoli fasci di luce. Il tipo varia in base alla Lastra che tiene.",  // JUDGMENT
  "Salta e sferra un calcio. Se fallisce, ferisce chi la usa.",  // JUMPKICK
  "Ripristina i PS e cura i problemi di stato di chi la usa e degli alleati.",  // JUNGLEHEALING
  "Colpo tagliente con alta probabilit\205 di brutto colpo.",  // KARATECHOP
  "Piega un cucchiaio per distrarre il bersaglio e ridurne la Precisione.",  // KINESIS
  "Protegge da ogni danno e riduce molto l'Attacco di chi lo tocca.",  // KINGSSHIELD
  "Impedisce al bersaglio di usare lo strumento che tiene per tutta la lotta.",  // KNOCKOFF
  "Si prostra in un inchino per far abbassare la guardia al bersaglio, poi attacca. Non manca mai.",  // KOWTOWCLEAVE
  "Raccoglie l'energia della crosta terrestre e la concentra contro i nemici.",  // LANDSWRATH
  "Si concentra affinch\202 il prossimo attacco sia un brutto colpo.",  // LASERFOCUS
  "Si pu\225 usare solo dopo aver usato tutte le altre mosse che il Pok\202mon conosce.",  // LASTRESORT
  "Vendica i compagni caduti. Pi\227 sono quelli andati KO, pi\227 la potenza aumenta.",  // LASTRESPECTS
  "Un inferno di fiamme colpisce i Pok\202mon adiacenti. Pu\225 scottare.",  // LAVAPLUME
  "Attacca il nemico lanciandogli foglie.",  // LEAFAGE
  "Taglia con una foglia sottile come una lama. Alta probabilit\205 di brutto colpo.",  // LEAFBLADE
  "Scatena una tempesta di foglie affilate. Riduce molto l'Attacco Speciale di chi la usa.",  // LEAFSTORM
  "Tritura con foglie affilate. Pu\225 ridurre la Precisione del bersaglio.",  // LEAFTORNADO
  "Recupera met\205 dei danni inflitti al bersaglio.",  // LEECHLIFE
  "Pianta semi che assorbono PS dal bersaglio a ogni turno per curare chi la usa.",  // LEECHSEED
  "Intimidisce il bersaglio con lo sguardo e ne riduce la Difesa.",  // LEER
  "Colpisce il bersaglio con la lingua. Pu\225 paralizzare.",  // LICK
  "Spruzza un'acqua misteriosa che ripristina PS a chi la usa e agli alleati.",  // LIFEDEW
  "Barriera di luce che per 5 turni riduce i danni degli attacchi speciali.",  // LIGHTSCREEN
  "Colpisce con la forza dell'acqua. Pu\225 ridurre la Difesa del bersaglio.",  // LIQUIDATION
  "Prende la mira affinch\202 il prossimo attacco non fallisca.",  // LOCKON
  "Con una faccia spaventosa bacia il bersaglio e lo fa addormentare.",  // LOVELYKISS
  "Calcio basso. Pi\227 pesa il bersaglio, pi\227 danni infligge.",  // LOWKICK
  "Rapido attacco ai piedi che riduce la Velocit\205 del bersaglio.",  // LOWSWEEP
  "Lancia al cielo un incantesimo che impedisce al bersaglio di infliggere brutti colpi.",  // LUCKYCHANT
  "Proietta una strana luce che colpisce la mente e riduce molto la Dif. Sp.",  // LUMINACRASH
  "Una preghiera alla luna che ripristina PS e cura i problemi di stato della squadra.",  // LUNARBLESSING
  "Chi la usa va KO, ma il Pok\202mon che lo sostituisce recupera stato, PS e PP.",  // LUNARDANCE
  "Si avventa sul nemico con tutte le sue forze e ne riduce l'Attacco.",  // LUNGE
  "Lampo di luce che pu\225 ridurre la Difesa Speciale del bersaglio.",  // LUSTERPURGE
  "Pugno rapidissimo che colpisce per primo.",  // MACHPUNCH
  "Sparge strane foglie che inseguono il bersaglio. Non si pu\225 evitare.",  // MAGICALLEAF
  "Il nemico resta intrappolato in una tempesta di fuoco da 2 a 5 turni.",  // MAGMASTORM
  "Lancia bombe di ferro che si attaccano al nemico. Non manca mai il bersaglio.",  // MAGNETBOMB
  "Levita per 5 turni grazie a un campo magnetico generato dall'elettricit\205.",  // MAGNETRISE
  "Scossa sismica di intensit\205 variabile che colpisce i Pok\202mon adiacenti.",  // MAGNITUDE
  "Lancia una pioggia di monete, ma riduce l'Att. Sp. Aumenta il denaro vinto.",  // MAKEITRAIN
  "Avvolge il bersaglio in una catena che ne mina le forze. Pu\225 iperavvelenarlo.",  // MALIGNANTCHAIN
  "Spruzza t\212 sul bersaglio e recupera PS pari a met\205 dei danni inflitti. Pu\225 scottare.",  // MATCHAGOTCHA
  "Impedisce al bersaglio di fuggire dalla lotta.",  // MEANLOOK
  "Medita in tranquillit\205 per aumentare l'Attacco.",  // MEDITATE
  "Assorbe met\205 dei danni inflitti.",  // MEGADRAIN
  "Violenta carica con corna imponenti.",  // MEGAHORN
  "Calcio di forza straordinaria.",  // MEGAKICK
  "Un pugno di grande potenza.",  // MEGAPUNCH
  "Chi la usa va KO, ma riduce molto Attacco e Attacco Speciale del bersaglio.",  // MEMENTO
  "Restituisce al bersaglio l'ultimo attacco subito, ma con molta pi\227 forza.",  // METALBURST
  "Attacco con artigli d'acciaio. Pu\225 aumentare l'Attacco di chi la usa.",  // METALCLAW
  "Orribile stridio metallico che riduce molto la Difesa Speciale del bersaglio.",  // METALSOUND
  "Colpisce con il suo porro. Al turno dopo chi la usa non pu\225 muoversi.",  // METEORASSAULT
  "1\370 turno: aumenta l'Att. Sp. 2\370 turno: attacca il bersaglio.",  // METEORBEAM
  "Pugno che colpisce come un meteorite. Pu\225 aumentare l'Attacco di chi la usa.",  // METEORMASH
  "Taglia il bersaglio con la luce accumulata sulla testa. Colpisce anche se si protegge.",  // MIGHTYCLEAVE
  "Ripristina met\205 dei PS massimi. Fuori dalla lotta pu\225 cedere PS a un alleato.",  // MILKDRINK
  "Fa esplodere la testa per colpire tutti intorno, ma ferisce anche se stesso.",  // MINDBLOWN
  "Intuisce i movimenti del bersaglio affinch\202 il prossimo attacco non fallisca.",  // MINDREADER
  "Rimpicciolisce per aumentare molto l'Elusione.",  // MINIMIZE
  "Permette di colpire i Pok\202mon di tipo Buio con qualsiasi mossa e quelli elusivi.",  // MIRACLEEYE
  "Risponde a un attacco speciale infliggendo il doppio dei danni subiti.",  // MIRRORCOAT
  "Contrattacca con la stessa mossa usata dal bersaglio.",  // MIRRORMOVE
  "Libera un fascio di energia dal corpo lucidato. Pu\225 ridurre la Precisione.",  // MIRRORSHOT
  "Avvolge chi la usa in una nebbia sottile che protegge le statistiche della squadra per 5 turni.",  // MIST
  "Banco di nebbia che pu\225 ridurre l'Attacco Speciale del bersaglio.",  // MISTBALL
  "Nebbia misteriosa con priorit\205 pi\227 alta sul Campo Nebbioso.",  // MISTYEXPLOSION
  "Copre il terreno di nebbia per 5 turni. I Pok\202mon a terra non subiscono problemi di stato.",  // MISTYTERRAIN
  "Invoca il potere della luna per attaccare. Pu\225 ridurre l'Attacco Speciale del bersaglio.",  // MOONBLAST
  "Colpisce con un misterioso raggio di energia lunare. Ignora l'abilit\205 del bersaglio.",  // MOONGEISTBEAM
  "Ripristina PS a chi la usa. La quantit\205 varia in base al tempo atmosferico.",  // MOONLIGHT
  "Ripristina PS a chi la usa. La quantit\205 varia in base al tempo atmosferico.",  // MORNINGSUN
  "Attacco rotante che avvelena il bersaglio e annulla effetti come Legatutto, Avvolgibotta e Parassiseme.",  // MORTALSPIN
  "Attacca con ghiaccioli grandi come iceberg. Pu\225 far tentennare il bersaglio.",  // MOUNTAINGALE
  "Lancia una compatta palla di fango. Pu\225 ridurre la Precisione del bersaglio.",  // MUDBOMB
  "Attacco di acqua fangosa che pu\225 ridurre la Precisione.",  // MUDDYWATER
  "Lancia fango sul bersaglio e ne riduce la Velocit\205.",  // MUDSHOT
  "Getta fango in faccia per ridurre la Precisione.",  // MUDSLAP
  "Si ricopre di fango e indebolisce le mosse di tipo Elettro finch\202 resta in lotta.",  // MUDSPORT
  "Colpisce avvolto da un'energia potente. Il tipo dipende dal disco che tiene.",  // MULTIATTACK
  "Soffia sul nemico una fiamma rovente che ne riduce l'Attacco Speciale.",  // MYSTICALFIRE
  "Scatena un potere misterioso che aumenta anche l'Attacco Speciale di chi la usa.",  // MYSTICALPOWER
  "Stimola il cervello con pensieri malvagi. Aumenta moltissimo l'Attacco Speciale.",  // NASTYPLOT
  "La Bacca che tiene presta la sua forza all'attacco. Tipo e potenza dipendono dalla Bacca.",  // NATURALGIFT
  "Colpisce il bersaglio con la furia della natura e ne dimezza i PS.",  // NATURESMADNESS
  "Colpisce con braccia spinose e pu\225 far tentennare il bersaglio.",  // NEEDLEARM
  "Colpisce il bersaglio con un'onda oscura. Pu\225 ridurne la Precisione.",  // NIGHTDAZE
  "Crea un miraggio davanti al bersaglio, che perde tanti PS quanto il livello di chi la usa.",  // NIGHTSHADE
  "Colpisce il bersaglio alla prima occasione. Alta probabilit\205 di brutto colpo.",  // NIGHTSLASH
  "Intimidisce il nemico con un grido di guerra e ne riduce Attacco e Attacco Speciale.",  // NOBLEROAR
  "Aumenta tutte le statistiche di chi la usa, ma non pu\225 pi\227 fuggire n\202 essere sostituito.",  // NORETREAT
  "Strofina le guance cariche di elettricit\205 sul bersaglio e lo paralizza.",  // NUZZLE
  "Assorbe energia dal bersaglio e recupera 3/4 dei danni inflitti.",  // OBLIVIONWING
  "Blocca ogni attacco e riduce molto la Difesa di chi lo colpisce con un contatto. Se usata spesso fallisce.",  // OBSTRUCT
  "Spruzza inchiostro in faccia. Pu\225 ridurre la Precisione.",  // OCTAZOOKA
  "Trattiene il bersaglio impedendogli di fuggire e ne riduce Difesa e Difesa Speciale a ogni turno.",  // OCTOLOCK
  "Permette di colpire i Pok\202mon di tipo Spettro con qualsiasi mossa e quelli elusivi.",  // ODORSLEUTH
  "Evoca un vento terrificante. Pu\225 aumentare insieme tutte le statistiche di chi la usa.",  // OMINOUSWIND
  "Se ha un Tatsugiri in bocca, aumenta una statistica in base alla forma di Tatsugiri.",  // ORDERUP
  "Colpisce il bersaglio con innumerevoli raggi di luce bluastra.",  // ORIGINPULSE
  "Attacca per 2 o 3 turni, poi chi la usa diventa confuso.",  // OUTRAGE
  "Pizzica il proprio corpo per generare enormi vibrazioni con cui colpisce il bersaglio.",  // OVERDRIVE
  "Attacco a piena potenza che riduce molto l'Attacco Speciale di chi lo usa.",  // OVERHEAT
  "Somma i PS di chi la usa a quelli del bersaglio e li divide in parti uguali.",  // PAINSPLIT
  "Danneggia i Pok\202mon intorno e assorbe met\205 dei danni inflitti.",  // PARABOLICCHARGE
  "Riduce Attacco e Attacco Speciale del nemico e si scambia con un Pok\202mon della squadra.",  // PARTINGSHOT
  "Contrattacca con potenza doppia se il bersaglio usa una mossa prima.",  // PAYBACK
  "Lancia monete al bersaglio. A fine lotta le recupera.",  // PAYDAY
  "Trafigge il bersaglio con un corno o un becco appuntito.",  // PECK
  "Scatena un'intensa bufera di petali che danneggia i Pok\202mon intorno.",  // PETALBLIZZARD
  "Lancia petali per 2 o 3 turni, poi chi la usa diventa confuso.",  // PETALDANCE
  "Sparisce al 1\370 turno e attacca al 2\370. Colpisce anche se il bersaglio si protegge.",  // PHANTOMFORCE
  "Confronta Attacco e Attacco Speciale di chi la usa e infligge danni con il pi\227 alto.",  // PHOTONGEYSER
  "Lancia aculei sottili che colpiscono da 2 a 5 volte.",  // PINMISSILE
  "Attacca con i pugni. Rende di tipo Elettro le mosse di tipo Normale.",  // PLASMAFISTS
  "Fa amicizia con il nemico togliendogli la voglia di lottare. Ne riduce anche l'Attacco.",  // PLAYNICE
  "Fa le moine al bersaglio e lo attacca. Pu\225 ridurne l'Attacco.",  // PLAYROUGH
  "Becca il bersaglio. Se tiene una Bacca, la becca anche lui e ne ottiene l'effetto.",  // PLUCK
  "Zanne tossiche che possono iperavvelenare il bersaglio.",  // POISONFANG
  "Spruzza una nube di gas tossico sul bersaglio e lo avvelena.",  // POISONGAS
  "Punge il nemico con un tentacolo o un braccio velenoso. Pu\225 avvelenarlo.",  // POISONJAB
  "Polvere tossica che avvelena il bersaglio.",  // POISONPOWDER
  "Lancia un pungiglione tossico che pu\225 avvelenare il bersaglio.",  // POISONSTING
  "Colpo di coda che pu\225 avvelenare. Alta probabilit\205 di brutto colpo.",  // POISONTAIL
  "Attacca con una sfera esplosiva. Se colpisce un alleato, gli fa recuperare PS.",  // POLLENPUFF
  "Attacca usando lo strumento tenuto dal nemico. Se non ne ha, fallisce.",  // POLTERGEIST
  "Attacco di gruppo che colpisce il bersaglio da 1 a 10 volte di fila.",  // POPULATIONBOMB
  "Si avventa sul bersaglio e ne riduce la Velocit\205.",  // POUNCE
  "Colpisce con le zampe o la coda.",  // POUND
  "Lancia neve che pu\225 congelare.",  // POWDERSNOW
  "Colpisce con un raggio di luce che scintilla come fosse fatto di mille gioielli.",  // POWERGEM
  "Pi\227 sono aumentate le statistiche di chi la usa, pi\227 danni infligge.",  // POWERTRIP
  "A ogni colpo i pugni si induriscono. Se va a segno, l'Attacco di chi la usa aumenta.",  // POWERUPPUNCH
  "Agita violentemente liane o tentacoli per colpire il bersaglio.",  // POWERWHIP
  "Manifesta il potere latente della terra come lame affilate e colpisce il bersaglio.",  // PRECIPICEBLADES
  "Regalo con una bomba trappola. A volte ripristina PS.",  // PRESENT
  "Emette un raggio potentissimo attraverso un prisma, ma al turno dopo non pu\225 muoversi.",  // PRISMATICLASER
  "Blocca gli attacchi, ma pu\225 fallire se usata pi\227 volte di fila.",  // PROTECT
  "Strano raggio che pu\225 confondere.",  // PSYBEAM
  "Squarcia il bersaglio con una spada eterea. La potenza aumenta sul Campo Elettrico.",  // PSYBLADE
  "Forte attacco telecinetico che pu\225 ridurre la Difesa Speciale del bersaglio.",  // PSYCHIC
  "Attacca con poteri psichici che distruggono anche barriere come Schermoluce e Riflesso.",  // PSYCHICFANGS
  "Per 5 turni le mosse di tipo Psico sono potenziate e quelle con priorit\205 falliscono.",  // PSYCHICTERRAIN
  "Attacco a piena potenza che riduce molto l'Attacco Speciale di chi lo usa.",  // PSYCHOBOOST
  "Colpisce con lame di energia psichica. Alta probabilit\205 di brutto colpo.",  // PSYCHOCUT
  "Si autoipnotizza e copia le modifiche alle statistiche del bersaglio.",  // PSYCHUP
  "Attacca avvolto da energia psichica, che aumenta anche la sua Difesa.",  // PSYSHIELDBASH
  "Crea un'onda psichica che colpisce il bersaglio e infligge gravi danni fisici.",  // PSYSHOCK
  "Crea un'onda psichica che colpisce il bersaglio infliggendo danni fisici.",  // PSYSTRIKE
  "Attacco con un'onda di energia di intensit\205 variabile.",  // PSYWAVE
  "Pi\227 il nemico ha aumentato le sue statistiche, pi\227 la potenza cresce.",  // PUNISHMENT
  "Cura i problemi di stato del nemico e in cambio recupera i propri PS.",  // PURIFY
  "Infligge danni doppi al bersaglio che si sta ritirando.",  // PURSUIT
  "Incendia un sassolino per creare una palla di fuoco con cui attacca. Pu\225 scottare.",  // PYROBALL
  "Attacco rapido che permette di colpire per primo.",  // QUICKATTACK
  "Protegge la squadra dalle mosse con priorit\205. Pu\225 fallire se usata pi\227 volte di fila.",  // QUICKGUARD
  "Danza mistica che aumenta Attacco Speciale, Difesa Speciale e Velocit\205.",  // QUIVERDANCE
  "Una volta usata, l'Attacco di chi la usa aumenta ogni volta che viene colpito.",  // RAGE
  "Trasforma la rabbia in energia. Pi\227 colpi ha subito, pi\227 la potenza aumenta.",  // RAGEFIST
  "Il tipo di questa mossa cambia in base alla forma di chi la usa. Distrugge le barriere.",  // RAGINGBULL
  "Attacca con fiamme violente per 2 o 3 turni di fila, poi diventa confuso.",  // RAGINGFURY
  "Scatena una forte pioggia che potenzia le mosse di tipo Acqua per 5 turni.",  // RAINDANCE
  "Attacco rotante che aumenta la Velocit\205 e rimuove effetti come Legatutto, Parassiseme o Punte.",  // RAPIDSPIN
  "Taglia con foglie affilate. Alta probabilit\205 di brutto colpo.",  // RAZORLEAF
  "Una conchiglia affilata colpisce il bersaglio. Pu\225 ridurne la Difesa.",  // RAZORSHELL
  "Attacca con vento tagliente. Alta probabilit\205 di brutto colpo.",  // RAZORWIND
  "Ripristina fino a met\205 dei PS massimi.",  // RECOVER
  "Ricicla uno strumento monouso gi\205 consumato nella lotta e lo recupera.",  // RECYCLE
  "Barriera di luce che per 5 turni riduce i danni degli attacchi fisici.",  // REFLECT
  "Riposa per curare paralisi, avvelenamento o scottature.",  // REFRESH
  "Commuove i nemici intorno con un antico canto. Pu\225 farli addormentare.",  // RELICSONG
  "Ripristina i PS e cura i problemi di stato di chi la usa, che dorme per i 2 turni seguenti.",  // REST
  "Se nel turno precedente un compagno \212 andato KO, la potenza dell'attacco aumenta.",  // RETALIATE
  "Pi\227 \212 alta l'amicizia con l'Allenatore, pi\227 questo attacco \212 potente.",  // RETURN
  "Danza energica del primo tipo di chi la usa. Se lo perde, usa il secondo.",  // REVELATIONDANCE
  "Infligge danni doppi se chi la usa \212 stato colpito nello stesso turno.",  // REVENGE
  "Attacco disperato che infligge pi\227 danni quanti meno PS ha chi la usa.",  // REVERSAL
  "La potenza raddoppia se il bersaglio \212 sul Campo Elettrico.",  // RISINGVOLTAGE
  "Spaventa il bersaglio e lo costringe a cambiare. Contro un Pok\202mon selvatico, la lotta finisce.",  // ROAR
  "Attacca con un'energia che distorce il tempo. Al turno dopo chi la usa deve riposare.",  // ROAROFTIME
  "Scaglia massi sul bersaglio da 2 a 5 volte di fila.",  // ROCKBLAST
  "Attacca con una grande carica. Pu\225 confondere il bersaglio.",  // ROCKCLIMB
  "Leviga il corpo per ridurre l'attrito. Aumenta molto la Velocit\205.",  // ROCKPOLISH
  "Scaglia grossi massi. Pu\225 far tentennare il bersaglio.",  // ROCKSLIDE
  "Colpisce con pugni capaci di frantumare le rocce. Pu\225 ridurre la Difesa del bersaglio.",  // ROCKSMASH
  "Lancia un piccolo masso contro il bersaglio.",  // ROCKTHROW
  "Lancia massi che bloccano il bersaglio e ne riducono la Velocit\205.",  // ROCKTOMB
  "Scaglia un masso enorme sul bersaglio, ma al turno dopo deve riposare.",  // ROCKWRECKER
  "Un calcio rapido e rotante. Pu\225 far tentennare il bersaglio.",  // ROLLINGKICK
  "Si arrotola e rotola come una ruota sul campo di lotta.",  // ROLLOUT
  "Atterra per riposare. Recupera fino a met\205 dei PS massimi.",  // ROOST
  "Un canto che colpisce il bersaglio. Pi\227 Pok\202mon lo usano, pi\227 diventa potente.",  // ROUND
  "Provoca una catastrofe devastante che dimezza i PS del bersaglio.",  // RUINATION
  "Fuoco mistico molto intenso che pu\225 scottare.",  // SACREDFIRE
  "Attacca con una spada ignorando le modifiche alle statistiche del bersaglio.",  // SACREDSWORD
  "Crea un campo protettivo che previene i problemi di stato per 5 turni.",  // SAFEGUARD
  "Il bersaglio perde PS a ogni turno. Colpisce di pi\227 i Pok\202mon di tipo Acciaio e Acqua.",  // SALTCURE
  "Getta sabbia in faccia e riduce la Precisione.",  // SANDATTACK
  "Avvolge il bersaglio in sabbia rovente che pu\225 scottarlo.",  // SANDSEARSTORM
  "Tempesta di sabbia che dura 5 turni e ferisce tutti tranne i tipi Roccia, Terra e Acciaio.",  // SANDSTORM
  "Intrappola il bersaglio in un vortice di sabbia da 4 a 5 turni.",  // SANDTOMB
  "Sparge semi per attaccare il bersaglio e gli pianta Parassiseme.",  // SAPPYSEED
  "Spara acqua bollente sul bersaglio. Pu\225 scottarlo.",  // SCALD
  "Spara squame da 2 a 5 volte. Aumenta la Velocit\205 di chi la usa ma ne riduce la Difesa.",  // SCALESHOT
  "Spaventa il bersaglio e ne riduce molto la Velocit\205.",  // SCARYFACE
  "Getta sabbia rovente sul bersaglio. Pu\225 scottarlo.",  // SCORCHINGSANDS
  "Graffia con artigli affilati.",  // SCRATCH
  "Stridio acuto che riduce molto la Difesa del bersaglio.",  // SCREECH
  "Un inferno di fiamme colpisce i Pok\202mon intorno. Pu\225 scottare.",  // SEARINGSHOT
  "Attacco dal potere segreto i cui effetti secondari variano in base all'ambiente.",  // SECRETPOWER
  "Trafigge il bersaglio con una sciabola e lo ferisce col suo potere mistico.",  // SECRETSWORD
  "Fa piovere sul bersaglio una raffica di semi esplosivi dall'alto.",  // SEEDBOMB
  "Libera un'onda d'urto dal corpo. Pu\225 ridurre la Difesa Speciale del bersaglio.",  // SEEDFLARE
  "Abbatte il bersaglio con la gravit\205. Toglie tanti PS quanto il livello di chi la usa.",  // SEISMICTOSS
  "Esplode e ferisce tutti i Pok\202mon intorno. Chi la usa va subito KO.",  // SELFDESTRUCT
  "Lancia una sfera oscura che pu\225 ridurre la Difesa Speciale del bersaglio.",  // SHADOWBALL
  "Colpisce con un osso posseduto da uno spirito. Pu\225 ridurre la Difesa del bersaglio.",  // SHADOWBONE
  "Colpisce con un artiglio affilato fatto d'ombra. Alta probabilit\205 di brutto colpo.",  // SHADOWCLAW
  "1\370 turno: sparisce. 2\370 turno: colpisce il bersaglio anche se si protegge.",  // SHADOWFORCE
  "Pugno inevitabile che arriva dalle ombre.",  // SHADOWPUNCH
  "Allunga la sua ombra e colpisce il bersaglio alle spalle. Colpisce sempre per primo.",  // SHADOWSNEAK
  "Rende pi\227 affilato il proprio profilo e aumenta l'Attacco.",  // SHARPEN
  "Attacco di gelo polare che manda KO il bersaglio, se va a segno.",  // SHEERCOLD
  "Rompe il guscio: riduce Difesa e Dif. Sp., ma aumenta molto Attacco, Att. Sp. e Velocit\205.",  // SHELLSMASH
  "Se subisce un attacco fisico, la trappola del guscio esplode e danneggia l'aggressore.",  // SHELLTRAP
  "Attacco elettrico rapidissimo e inevitabile.",  // SHOCKWAVE
  "Ripristina met\205 dei PS massimi. Durante una tempesta di sabbia ne ripristina di pi\227.",  // SHOREUP
  "Attacca con un sinistro raggio di luce. Pu\225 confondere il bersaglio.",  // SIGNALBEAM
  "Trappola di seta che protegge chi la usa. Riduce la Velocit\205 di chi la tocca.",  // SILKTRAP
  "Forte vento carico di scaglie. Pu\225 aumentare tutte le statistiche di chi lo usa.",  // SILVERWIND
  "Una canzoncina che fa addormentare profondamente il bersaglio.",  // SING
  "Si avvolge di fuoco e carica il bersaglio. Lo scotta sempre.",  // SIZZLYSLIDE
  "Impara per sempre l'ultima mossa del bersaglio. Si pu\225 usare una sola volta.",  // SKETCH
  "Attacca il bersaglio di soppiatto alle spalle e ne riduce l'Attacco Speciale.",  // SKITTERSMACK
  "1\370 turno: si prepara e aumenta la Difesa. 2\370 turno: sferra la testata.",  // SKULLBASH
  "Attacco in 2 turni con alta probabilit\205 di brutto colpo. Pu\225 far tentennare il bersaglio.",  // SKYATTACK
  "Montante verso l'alto di grande impeto.",  // SKYUPPERCUT
  "Si rilassa e ripristina met\205 dei PS massimi.",  // SLACKOFF
  "Colpisce con gli arti.",  // SLAM
  "Attacca con lame o chele. Alta probabilit\205 di brutto colpo.",  // SLASH
  "Sparge una polvere che fa addormentare il bersaglio.",  // SLEEPPOWDER
  "Lancia rifiuti sul bersaglio. Pu\225 avvelenarlo.",  // SLUDGE
  "Lancia rifiuti sul bersaglio. Pu\225 avvelenarlo.",  // SLUDGEBOMB
  "Un'onda tossica che danneggia i Pok\202mon intorno. Pu\225 avvelenare.",  // SLUDGEWAVE
  "Lancia una pietra o un proiettile. Se il bersaglio vola, lo fa cadere a terra.",  // SMACKDOWN
  "Trafigge il nemico con le corna affilate. Non manca mai il bersaglio.",  // SMARTSTRIKE
  "Infligge danni doppi ai bersagli paralizzati, ma ne cura la paralisi.",  // SMELLINGSALTS
  "Colpisce il bersaglio con gas tossici che possono avvelenarlo.",  // SMOG
  "Riduce la Precisione del bersaglio con una nube di fumo o inchiostro.",  // SMOKESCREEN
  "Intrappola il bersaglio in una tagliola da 4 a 5 turni.",  // SNAPTRAP
  "Lancia un urlo insopportabile che riduce l'Attacco Speciale dei bersagli.",  // SNARL
  "Colpisce il bersaglio ignorando abilit\205 o mosse che deviano gli attacchi.",  // SNIPESHOT
  "Forte russata da usare mentre si dorme. Pu\225 far tentennare il bersaglio.",  // SNORE
  "Fa nevicare per 5 turni e aumenta la Difesa dei Pok\202mon di tipo Ghiaccio.",  // SNOWSCAPE
  "Una pioggia potente che rende il bersaglio di tipo Acqua.",  // SOAK
  "Ripristina met\205 dei PS massimi. Fuori dalla lotta cede PS a un alleato.",  // SOFTBOILED
  "1\370 turno: assorbe luce. 2\370 turno: attacca.",  // SOLARBEAM
  "Al 1\370 turno concentra energia a forma di lama e attacca al 2\370 turno.",  // SOLARBLADE
  "Lancia onde d'urto che tolgono sempre 20 PS al bersaglio.",  // SONICBOOM
  "Squarcia il bersaglio e lo spazio intorno. Alta probabilit\205 di brutto colpo.",  // SPACIALREND
  "Attacco elettrico che pu\225 paralizzare.",  // SPARK
  "Libera bolle cantando. Cura le scottature dei Pok\202mon colpiti.",  // SPARKLINGARIA
  "Avvolge il bersaglio in un vortice. Cura tutti i problemi di stato della squadra.",  // SPARKLYSWIRL
  "Si nasconde nell'ombra del bersaglio, ne ruba i potenziamenti alle statistiche e attacca.",  // SPECTRALTHIEF
  "Avvolge il bersaglio per impedirgli di lasciare la lotta.",  // SPIDERWEB
  "Lancia aculei sottili che colpiscono da 2 a 5 volte.",  // SPIKECANNON
  "Sparge punte sul campo nemico. Feriscono i Pok\202mon nemici che entrano in campo.",  // SPIKES
  "Evita l'attacco del nemico e lo danneggia se lo ha toccato.",  // SPIKYSHIELD
  "Ruota violentemente su se stesso. Riduce molto la Velocit\205 di chi la usa.",  // SPINOUT
  "Attacca con tale impeto da fiaccare il morale del bersaglio e ridurne l'Attacco Speciale.",  // SPIRITBREAK
  "Attacca il nemico e ne inchioda l'ombra al suolo, impedendogli di fuggire.",  // SPIRITSHACKLE
  "Sfoga il suo rancore per togliere 4 PP all'ultima mossa usata dal nemico.",  // SPITE
  "Libera in un colpo l'energia accumulata con Accumulo.",  // SPITUP
  "Non ha alcun effetto. Si limita a sguazzare.",  // SPLASH
  "Sparge spore che fanno addormentare.",  // SPORE
  "Scatena una tempesta di amore e odio che avvolge il bersaglio. Pu\225 ridurne l'Attacco.",  // SPRINGTIDESTORM
  "Una trappola di rocce fluttuanti che danneggia i nemici che entrano in campo.",  // STEALTHROCK
  "Avvolge il nemico in vapore rovente che pu\225 scottarlo.",  // STEAMERUPTION
  "Si appallottola e travolge il bersaglio col corpo. Pu\225 far tentennare.",  // STEAMROLLER
  "Spara un raggio di metallo raccolto dal proprio corpo. Danneggia chi la usa.",  // STEELBEAM
  "Attacca e distrugge il campo attivo. Fallisce se non ce n'\212 nessuno.",  // STEELROLLER
  "Ali robuste che colpiscono il bersaglio. Pu\225 aumentare la Difesa di chi la usa.",  // STEELWING
  "Stende una rete appiccicosa nel campo nemico che riduce la Velocit\205 di chi entra.",  // STICKYWEB
  "Accumula energia e aumenta Difesa e Difesa Speciale. Si pu\225 usare fino a 3 volte.",  // STOCKPILE
  "Pestone tremendo che pu\225 far tentennare il bersaglio.",  // STOMP
  "Sfoga la frustrazione. Potenza doppia se l'ultima mossa di chi la usa \212 fallita.",  // STOMPINGTANTRUM
  "Colpisce con un'ascia di pietra, lasciando schegge che circondano il bersaglio.",  // STONEAXE
  "Conficca pietre affilate nel bersaglio. Alta probabilit\205 di brutto colpo.",  // STONEEDGE
  "Pi\227 sono aumentate le statistiche di chi la usa, pi\227 danni infligge.",  // STOREDPOWER
  "Sferra un colpo fulminante. Infligge sempre un brutto colpo.",  // STORMTHROW
  "Attacca il nemico emettendo vapore. Pu\225 confondere il bersaglio.",  // STRANGESTEAM
  "Pugno potente.",  // STRENGTH
  "Riduce l'Attacco del nemico e ripristina PS pari al valore del suo Attacco.",  // STRENGTHSAP
  "Lancia seta sul bersaglio e ne riduce la Velocit\205.",  // STRINGSHOT
  "Contrattacca e riduce anche l'Attacco Speciale dei nemici.",  // STRUGGLEBUG
  "Sparge una polvere che paralizza il bersaglio.",  // STUNSPORE
  "Getta a terra il bersaglio. Ferisce anche chi la usa.",  // SUBMISSION
  "Permette di colpire per primo. Fallisce se il bersaglio non sta preparando un attacco.",  // SUCKERPUNCH
  "Intensifica il sole per 5 turni e potenzia le mosse di tipo Fuoco.",  // SUNNYDAY
  "Colpisce il bersaglio con la potenza di una meteora.",  // SUNSTEELSTRIKE
  "Elettrifica il corpo e si tuffa sul bersaglio. Se fallisce, ferisce chi la usa.",  // SUPERCELLSLAM
  "Zanne affilate che dimezzano i PS del bersaglio.",  // SUPERFANG
  "Attacco molto potente, ma riduce Attacco e Difesa di chi lo usa.",  // SUPERPOWER
  "Strane onde sonore che possono confondere il bersaglio.",  // SUPERSONIC
  "Inonda il campo di lotta con un'onda gigantesca.",  // SURF
  "Confonde il bersaglio, ma ne aumenta molto l'Attacco.",  // SWAGGER
  "Assorbe l'energia accumulata con Accumulo e ripristina PS.",  // SWALLOW
  "D\205 un bacio cos\215 dolce da confondere.",  // SWEETKISS
  "Ammalia il nemico e ne riduce l'Elusione. Attira anche i Pok\202mon selvatici.",  // SWEETSCENT
  "Spara raggi a forma di stella che non mancano mai.",  // SWIFT
  "Scambia lo strumento tenuto con il bersaglio, cos\215 in fretta che l'occhio non lo vede.",  // SWITCHEROO
  "Danza frenetica che aumenta molto l'Attacco.",  // SWORDSDANCE
  "Strana onda che danneggia tutti i Pok\202mon adiacenti dello stesso tipo di chi la usa.",  // SYNCHRONOISE
  "Ripristina PS a chi la usa. La quantit\205 varia in base al tempo atmosferico.",  // SYNTHESIS
  "Ricopre il bersaglio di un nettare viscoso che ne riduce la Velocit\205 per 3 turni.",  // SYRUPBOMB
  "Raffica di lame di particelle che colpisce 2 volte di fila. Non manca mai.",  // TACHYONCUTTER
  "Carica il bersaglio con il corpo.",  // TACKLE
  "Si concentra su un lampo di luce che aumenta moltissimo l'Attacco Speciale.",  // TAILGLOW
  "Colpisce con la coda o altre parti del corpo da 2 a 5 volte di fila.",  // TAILSLAP
  "Agita la coda per ridurre la Difesa del nemico.",  // TAILWHIP
  "Un forte vento a favore raddoppia la Velocit\205 dei Pok\202mon della squadra per 4 turni.",  // TAILWIND
  "Carica sfrenata che ferisce anche chi la usa.",  // TAKEDOWN
  "Cura i propri problemi di stato e aumenta Attacco Speciale e Difesa Speciale.",  // TAKEHEART
  "Getta catrame sul bersaglio, ne riduce la Velocit\205 e lo rende vulnerabile alle mosse di tipo Fuoco.",  // TARSHOT
  "Fa infuriare il bersaglio, che per 3 turni pu\225 usare solo mosse d'attacco.",  // TAUNT
  "Fissa il bersaglio facendogli perdere lo spirito combattivo. Ne riduce Attacco e Attacco Speciale.",  // TEARFULLOOK
  "Colpisce con un grande laser. Il tipo dipende dal Modulo che tiene chi la usa.",  // TECHNOBLAST
  "Danza sgangherata che confonde i Pok\202mon intorno a chi la usa.",  // TEETERDANCE
  "Se la usa Terapagos in Forma Astrale, danneggia tutti i nemici.",  // TERASTARSTORM
  "Sfrutta l'energia del campo attivo, che ne determina tipo e potenza.",  // TERRAINPULSE
  "Attacca e ruba lo strumento del bersaglio. Se chi la usa tiene gi\205 uno strumento, non ruba.",  // THIEF
  "Colpisce anche i Pok\202mon in volo e li fa cadere a terra.",  // THOUSANDARROWS
  "Onde sismiche si propagano nel suolo verso tutti i nemici. Chi viene colpito non pu\225 fuggire.",  // THOUSANDWAVES
  "Attacca per 2 o 3 turni, poi chi la usa diventa confuso.",  // THRASH
  "Infligge un dolore cos\215 forte che il nemico non pu\225 usare mosse sonore per 2 turni.",  // THROATCHOP
  "Un potente fulmine che danneggia il bersaglio e pu\225 paralizzarlo.",  // THUNDER
  "Potente attacco elettrico che pu\225 paralizzare il bersaglio.",  // THUNDERBOLT
  "Intrappola il nemico in una gabbia elettrica che dura da 4 a 5 turni.",  // THUNDERCAGE
  "Un fulmine colpisce il bersaglio con priorit\205. Fallisce se il bersaglio non attacca.",  // THUNDERCLAP
  "Morde con zanne elettrificate. Pu\225 far tentennare o paralizzare il nemico.",  // THUNDERFANG
  "Pugno elettrico. Pu\225 paralizzare.",  // THUNDERPUNCH
  "Attacco elettrico che pu\225 paralizzare il bersaglio.",  // THUNDERSHOCK
  "Una leggera scarica che paralizza il bersaglio, se va a segno.",  // THUNDERWAVE
  "Fa ridere il bersaglio per ridurne Attacco e Difesa.",  // TICKLE
  "Aumenta Attacco e Velocit\205. Rimuove effetti come Punte, Levitoroccia o Sostituto.",  // TIDYUP
  "Emette fiammate roventi come se cantasse. Aumenta l'Attacco Speciale.",  // TORCHSONG
  "Tormenta il bersaglio, che non pu\225 usare la stessa mossa 2 volte di fila.",  // TORMENT
  "Iperavvelena il bersaglio. I danni da veleno aumentano a ogni turno.",  // TOXIC
  "Sparge punte tossiche sul campo nemico. Il veleno colpisce i Pok\202mon che entrano in campo.",  // TOXICSPIKES
  "Attacca con fili velenosi che riducono la Velocit\205 del bersaglio e lo avvelenano.",  // TOXICTHREAD
  "Si trasforma in una copia del bersaglio, con le stesse mosse.",  // TRANSFORM
  "Attacco triplo che pu\225 paralizzare, scottare o congelare il bersaglio.",  // TRIATTACK
  "Inganna il bersaglio distratto e si scambia gli strumenti con lui.",  // TRICK
  "Un triplo tuffo perfettamente sincronizzato che colpisce il bersaglio 3 volte di fila.",  // TRIPLEDIVE
  "Colpisce fino a 3 volte di fila, ogni volta pi\227 forte.",  // TRIPLEHIT
  "Calcia fino a 3 volte di fila, ogni volta pi\227 forte.",  // TRIPLEKICK
  "Sferra un calcio con la forza dei tropici che colpisce il nemico e ne riduce l'Attacco.",  // TROPKICK
  "Meno PP restano alla mossa, pi\227 l'attacco \212 potente.",  // TRUMPCARD
  "Emette dagli occhi due misteriosi raggi di luce che colpiscono 2 volte di fila.",  // TWINBEAM
  "Punge il nemico 2 volte con aculei. Pu\225 avvelenare.",  // TWINEEDLE
  "Crea un tornado violento che fa a pezzi il nemico. Pu\225 farlo tentennare.",  // TWISTER
  "Fa baccano per 3 turni. Tiene tutti svegli.",  // UPROAR
  "Dopo l'attacco torna indietro in fretta e lascia il posto a un compagno di squadra.",  // UTURN
  "Ruota i pugni e scaglia un'onda di vuoto sul bersaglio. Colpisce sempre per primo.",  // VACUUMWAVE
  "Colpisce con una V di fiamme. Riduce Difesa, Dif. Sp. e Velocit\205 di chi la usa.",  // VCREATE
  "Riduce Attacco, Attacco Speciale e Velocit\205, ma solo ai Pok\202mon gi\205 avvelenati.",  // VENOMDRENCH
  "Inzuppa il bersaglio di un liquido velenoso. Danni doppi se \212 gi\205 avvelenato.",  // VENOSHOCK
  "Sferza il bersaglio con rami sottili.",  // VINEWHIP
  "Afferra con potenti chele.",  // VISEGRIP
  "Chi la usa attacca per ultimo, ma non manca mai.",  // VITALTHROW
  "Dopo l'attacco torna indietro in fretta e lascia il posto a un compagno di squadra.",  // VOLTSWITCH
  "Carica ad alto rischio che ferisce anche chi la usa.",  // VOLTTACKLE
  "Infligge gravi danni ai bersagli addormentati, ma gli schiaffi li svegliano.",  // WAKEUPSLAP
  "Carica con grande impeto. Pu\225 far tentennare. Serve anche a risalire le cascate.",  // WATERFALL
  "Spara acqua con grande potenza.",  // WATERGUN
  "Colpisce con un potente getto d'acqua. Pu\225 confondere il bersaglio.",  // WATERPULSE
  "Colpisce da 2 a 5 volte con stelle da lancio. Colpisce sempre per primo.",  // WATERSHURIKEN
  "Si bagna e, finch\202 resta in lotta, le mosse di tipo Fuoco si indeboliscono.",  // WATERSPORT
  "Getto d'acqua. Meno PS ha chi la usa, meno danni infligge.",  // WATERSPOUT
  "Si avvolge d'acqua e carica il bersaglio, ma subisce anche gravi danni.",  // WAVECRASH
  "Il tipo e la potenza dell'attacco variano in base al tempo atmosferico.",  // WEATHERBALL
  "Un vortice d'acqua intrappola il bersaglio per 4 o 5 turni.",  // WHIRLPOOL
  "Spazza via il nemico, che viene sostituito. Contro un Pok\202mon selvatico, la lotta finisce.",  // WHIRLWIND
  "Colpo devastante che richiede la padronanza assoluta delle arti oscure. Infligge sempre un brutto colpo.",  // WICKEDBLOW
  "Evoca una tempesta che colpisce con forti venti e fulmini. Pu\225 paralizzare il bersaglio.",  // WILDBOLTSTORM
  "Carica elettrica potentissima che ferisce leggermente anche chi la usa.",  // WILDCHARGE
  "Sinistra fiamma viola che scotta il bersaglio.",  // WILLOWISP
  "Colpisce il bersaglio con grandi ali.",  // WINGATTACK
  "Si ritira nel guscio e aumenta la Difesa.",  // WITHDRAW
  "Si scaglia sul nemico con il suo corpo robusto. Anche chi la usa subisce parecchi danni.",  // WOODHAMMER
  "Aumenta Attacco e Attacco Speciale di chi la usa.",  // WORKUP
  "Pianta nel bersaglio un seme che lo angoscia e gli d\205 l'abilit\205 Insonnia.",  // WORRYSEED
  "Stritola il bersaglio da 4 a 5 turni con rami o con il corpo.",  // WRAP
  "Strizza con forza il bersaglio. Pi\227 PS ha il bersaglio, pi\227 l'attacco \212 potente.",  // WRINGOUT
  "Incrocia falci o artigli per colpire il nemico come un paio di forbici.",  // XSCISSOR
  "Un grande sbadiglio che fa addormentare il bersaglio al turno successivo.",  // YAWN
  "Spara una scarica elettrica che infligge danni e paralizza.",  // ZAPCANNON
  "Concentra l'energia psichica nella testa per colpire. Pu\225 far tentennare il bersaglio.",  // ZENHEADBUTT
  "Si lancia sul bersaglio e rilascia una potente scarica. Pu\225 farlo tentennare.",  // ZINGZAP
  "Impedisce al bersaglio di usare lo strumento che tiene. Il suo Allenatore non pu\225 usargli strumenti.",  // EMBARGO
  "Per 5 turni il bersaglio non pu\225 usare mosse, abilit\205 o strumenti che recuperano PS.",  // HEALBLOCK
  "Se un Pok\202mon sente questo canto e non viene sostituito entro 3 turni, va KO.",  // PERISHSONG
  "Usa parte dei propri PS per creare un sostituto che fa da esca.",  // SUBSTITUTE
  "Crea uno spazio strano dove per 5 turni i Pok\202mon lenti si muovono per primi.",  // TRICKROOM
  "Esprime un desiderio che ripristina met\205 dei PS massimi al turno successivo.",  // WISH
};

static const char *const MOVE_DESC_JA[MOVE_COUNT] = {
  "だせる わざが ない ときに つかう。 じぶんも ダメージを うける。",  // STRUGGLE
  "あたえた ダメージの はんぶんの HPを すいとる。",  // ABSORB
  "すごい はやさで あいてに ぶつかる。 かならず せんせい できる。",  // ACCELEROCK
  "つよい さんを かける。 あいての とくぼうを さげる ことが ある。",  // ACID
  "からだを えきたいに かえて ぼうぎょを ぐーんと あげる。",  // ACIDARMOR
  "とかす えきたいで こうげき。 あいての とくぼうを がくっと さげる。",  // ACIDSPRAY
  "すばやく こうげき。 どうぐを もっていないと おおきな ダメージを あたえる。",  // ACROBATICS
  "からだの つぼを おして のうりょくの どれか 1つを ぐーんと あげる。",  // ACUPRESSURE
  "すばやい うごきで こうげき。 かならず あたる。",  // AERIALACE
  "くうきの うずを うちだす。 きゅうしょに あたりやすい。",  // AEROBLAST
  "からだを リラックス させて すばやさを ぐーんと あげる。",  // AGILITY
  "するどい かぜで きりつける。 きゅうしょに あたりやすい。",  // AIRCUTTER
  "そらをも きる くうきの やいばで こうげき。 ひるませる ことが ある。",  // AIRSLASH
  "あたまを からっぽに して とくぼうを ぐーんと あげる。",  // AMNESIA
  "いかりを なげつけて こうげき。 あいては にげられなくなる。",  // ANCHORSHOT
  "げんしの ちからで こうげき。 じぶんの のうりょくが ぜんぶ あがる ことが ある。",  // ANCIENTPOWER
  "すっぱい りんごの さんで こうげき。 あいての とくぼうを さげる。",  // APPLEACID
  "あつりょくを かけた みずを やいばの ように して きる。 きゅうしょに あたりやすい。",  // AQUACUTTER
  "すばやい こうげきで かならず せんせい できる。",  // AQUAJET
  "みずの ベールで からだを つつむ。 まいターン HPが すこし かいふくする。",  // AQUARING
  "かろやかに おどって こうげき。 じぶんの すばやさが あがる。",  // AQUASTEP
  "あれくるう なみの ように しっぽを ふって こうげきする。",  // AQUATAIL
  "もえる よろいを うちだす。 じぶんの ぼうぎょと とくぼうが さがる。",  // ARMORCANNON
  "つっぱりで 2から 5かい つづけて こうげきする。",  // ARMTHRUST
  "やさしい かおりで みかた ぜんいんの じょうたいいじょうを なおす。",  // AROMATHERAPY
  "ふしぎな かおりで みかたの とくぼうを あげる。",  // AROMATICMIST
  "そのターン あいてが ダメージを うけていると いりょくが 2ばいに なる。",  // ASSURANCE
  "おどろかせて こうげき。 ひるませる ことが ある。",  // ASTONISH
  "むしタイプに よく きく あごで かみつく。",  // ATRAPAMOSCAS
  "てしたを よんで こうげき させる。 きゅうしょに あたりやすい。",  // ATTACKORDER
  "せいべつが ちがう あいてを メロメロに して こうげき しにくくする。",  // ATTRACT
  "はどうの ちからを うちだす。 かならず あたる。",  // AURASPHERE
  "こうげきして すばやさを あげる。 すがたで タイプが かわる。",  // AURAWHEEL
  "にじいろの ビーム。 あいての こうげきを さげる ことが ある。",  // AURORABEAM
  "からだを かるくして すばやさを ぐーんと あげる。",  // AUTOTOMIZE
  "そのターン じぶんに こうげきした あいてには 2ばいの ダメージ。",  // AVALANCHE
  "かかとを ふりおろす。 はずれると じぶんが ダメージ。 こんらん させる ことが ある。",  // AXEKICK
  "あいてを みつめて こうげきを さげる。 かならず せんせい できる。",  // BABYDOLLEYES
  "わるく ふるまって こうげきし リフレクターを はる。",  // BADDYBAD
  "こうげきを ふせぎ ふれた あいてを どくに する。",  // BANEFULBUNKER
  "どくばりで どくに する ことが ある。 あいてが どくだと いりょく 2ばい。",  // BARBBARRAGE
  "まるい ものを 2から 5かい つづけて なげつける。",  // BARRAGE
  "がんじょうな かべを つくり ぼうぎょを ぐーんと あげる。",  // BARRIER
  "みかたと いれかわり のうりょくの へんかを ひきつぐ。",  // BATONPASS
  "くちばしを あつくしてから こうげき。 そのあいだに ふれた あいては やけど。",  // BEAKBLAST
  "てもちの なかまが こうげきに くわわる。 おおいほど なんかいも あたる。",  // BEATUP
  "ゲップで ダメージ。 もっている きのみを たべないと だせない。",  // BELCH
  "さいだいHPの はんぶんを けずって こうげきを さいだいまで あげる。",  // BELLYDRUM
  "2ターン がまんして うけた ダメージを 2ばいに して かえす。",  // BIDE
  "からだや しょくしゅで 4から 5ターン しめつける。",  // BIND
  "するどい キバで かみつく。 ひるませる ことが ある。",  // BITE
  "うらみを こめて つきさす。 あたえた ダメージの はんぶんの HPを かいふく。",  // BITTERBLADE
  "つめたい うらみで こうげきし あいての こうげきを さげる。",  // BITTERMALICE
  "きゅうしょに あたりやすい キック。 やけどに する ことが ある。",  // BLAZEKICK
  "つめたい かぜで こうげき。 あいての すばやさを さげる ことが ある。",  // BLEAKWINDSTORM
  "はげしい ふぶき。 こおらせる ことが ある。",  // BLIZZARD
  "りょうてを ひろげて とおせんぼう。 あいては にげられなくなる。",  // BLOCK
  "うつくしくも はげしい あおい ほのお。 やけどに する ことが ある。",  // BLUEFLARE
  "からだを ぶつける。 じぶんの ぼうぎょが たかいほど ダメージが ふえる。",  // BODYPRESS
  "からだ ぜんたいで のしかかる。 まひ させる ことが ある。",  // BODYSLAM
  "でんきを まとった くちばしで つつく。 さきに こうげき できれば いりょく 2ばい。",  // BOLTBEAK
  "でんきを まとって たいあたり。 まひ させる ことが ある。",  // BOLTSTRIKE
  "ホネで なぐる。 ひるませる ことが ある。",  // BONECLUB
  "ホネを ブーメランの ように なげて 2かい あてる。",  // BONEMERANG
  "ホネを かまえて 2から 5かい つづけて なぐる。",  // BONERUSH
  "すさまじい おとの ちからで まわりの ポケモン すべてを こうげきする。",  // BOOMBURST
  "1ターンめに とびはね 2ターンめに こうげき。 まひ させる ことが ある。",  // BOUNCE
  "みずの あわで こうげきし あたえた ダメージの はんぶんを すいとる。",  // BOUNCYBUBBLE
  "するどい えだで あいてを つついて こうげきする。",  // BRANCHPOKE
  "つばさを たたみ ていくうで とっしん。 じぶんも ダメージを うける。",  // BRAVEBIRD
  "しっぽを はげしく たたきつけ あいての こうげきを さげる。",  // BREAKINGSWIPE
  "ひかりのかべや リフレクターも はかいする つよい こうげき。",  // BRICKBREAK
  "あいての HPが はんぶん いかだと 2ばいの ダメージを あたえる。",  // BRINE
  "からだを ふりまわして あいて ぜんいんに ダメージを あたえる。",  // BRUTALSWING
  "たくさんの あわを ふきつける。 すばやさを さげる ことが ある。",  // BUBBLE
  "あわを いきおいよく ふきつける。 すばやさを さげる ことが ある。",  // BUBBLEBEAM
  "かみついて こうげき。 あいてが きのみを もっていれば たべて こうかを えられる。",  // BUGBITE
  "はねの しんどうで おんぱを だす。 とくぼうを さげる ことが ある。",  // BUGBUZZ
  "からだを きたえて こうげきと ぼうぎょを あげる。",  // BULKUP
  "じめんを ゆらして まわりの ポケモンを こうげきし すばやさを さげる。",  // BULLDOZE
  "だんがんの ように はやい パンチ。 かならず せんせい できる。",  // BULLETPUNCH
  "タネを 2から 5かい つづけて すばやく うちだす。",  // BULLETSEED
  "けがわで こうげきを ふせぐ。 ふれた あいてを やけどに する ことが ある。",  // BURNINGBULWARK
  "ほのおを もやしつくして こうげき。 じぶんは ほのおタイプで なくなる。",  // BURNUP
  "でんきを はなって こうげき。 かならず まひ させる。",  // BUZZYBUZZ
  "こころを しずめて とくこうと とくぼうを あげる。",  // CALMMIND
  "せいべつが ちがう あいてを ゆうわくして とくこうを がくっと さげる。",  // CAPTIVATE
  "でんきを ためて つぎの でんきわざを つよくする。 とくぼうも あがる。",  // CHARGE
  "でんきの ビームで こうげき。 じぶんの とくこうが あがる ことが ある。",  // CHARGEBEAM
  "かわいく あまえて あいての こうげきを がくっと さげる。",  // CHARM
  "おぼえた ことばの おんぱで こうげき。 こんらん させる ことが ある。",  // CHATTER
  "つめたい みずを かけて あいての こうげきを さげる。",  // CHILLINGWATER
  "さむい ギャグで ゆきを ふらせ てもちの ポケモンと いれかわる。",  // CHILLYRECEPTION
  "すきを ついて こうげき。 あいての のうりょくの へんかを むしする。",  // CHIPAWAY
  "ようりょくそを ためて うちだす。 じぶんも ダメージを うける。",  // CHLOROBLAST
  "あいてを なげとばし こうたい させる。 やせいでは せんとうが おわる。",  // CIRCLETHROW
  "かたい からで 4から 5ターン はさみつける。",  // CLAMP
  "うろこを こすって おおきな おとで こうげき。 じぶんの ぼうぎょが さがる。",  // CLANGINGSCALES
  "HPを すこし けずって じぶんの のうりょくを ぜんぶ あげる。",  // CLANGOROUSSOUL
  "ふしぎな きりで こうげきし あいての のうりょくの へんかを もとに もどす。",  // CLEARSMOG
  "みを まもらず たたかう。 じぶんの ぼうぎょと とくぼうが さがる。",  // CLOSECOMBAT
  "とぐろを まいて こうげき ぼうぎょ めいちゅうりつを あげる。",  // COIL
  "へんけいしながら じめんに ぶつかる。 こうかばつぐんだと いりょくが あがる。",  // COLLISIONCOURSE
  "パンチを 2から 5かい つづけて くりだす。",  // COMETPUNCH
  "さいごに うけた こうげきを もっと つよくして かえす。",  // COMEUPPANCE
  "ないしょばなしで しゅうちゅうを みだし とくこうを さげる。",  // CONFIDE
  "あやしい ひかりで あいてを こんらん させる。",  // CONFUSERAY
  "よわい ねんりきで こうげき。 こんらん させる ことが ある。",  // CONFUSION
  "しょくしゅや ツタで こうげき。 すばやさを さげる ことが ある。",  // CONSTRICT
  "ちょくぜんに つかわれた わざを まねする。 まだ なければ しっぱいする。",  // COPYCAT
  "こうげきする。 あいてが すでに わざを つかっていたら とくせいを けす。",  // COREENFORCER
  "うちゅうの ちからで ぼうぎょと とくぼうを あげる。",  // COSMICPOWER
  "わたで からだを つつみ ぼうぎょを ぐぐーんと あげる。",  // COTTONGUARD
  "わたの ほうしを くっつけて すばやさを がくっと さげる。",  // COTTONSPORE
  "ぶつりわざを うけると 2ばいの ダメージで かえす。",  // COUNTER
  "あまえて ちかよって こうげき。 もちものを うばう ことが ある。",  // COVET
  "おおきな ハサミで たたく。 きゅうしょに あたりやすい。",  // CRABHAMMER
  "りょうてで チョップ。 きゅうしょに あたりやすい。",  // CROSSCHOP
  "どくの やいばで きる。 どくに する ことが あり きゅうしょに あたりやすい。",  // CROSSPOISON
  "するどい キバで かみくだく。 ぼうぎょを さげる ことが ある。",  // CRUNCH
  "するどい ツメで きりさく。 ぼうぎょを さげる ことが ある。",  // CRUSHCLAW
  "あいての HPが おおいほど いりょくが あがる。",  // CRUSHGRIP
  "ゴーストタイプか どうかで こうかが かわる わざ。",  // CURSE
  "ツメや カマで きる。 ほそい きを きる ことも できる。",  // CUT
  "まわって ラリアット。 あいての のうりょくの へんかを むしする。",  // DARKESTLARIAT
  "わるい こころの オーラを はなつ。 ひるませる ことが ある。",  // DARKPULSE
  "あいてを くらやみの せかいに おとし ねむらせる。",  // DARKVOID
  "つよい ひかりで あいてに ダメージを あたえる。",  // DAZZLINGGLEAM
  "てしたを よんで たてに する。 ぼうぎょと とくぼうが あがる。",  // DEFENDORDER
  "まるく なって じゃくてんを かくし ぼうぎょを あげる。",  // DEFENSECURL
  "リフレクターや ひかりのかべを ふきとばす。 かいひりつを さげる ことが ある。",  // DEFOG
  "スキーで すべって こうげき。 じぶんの すばやさを あげる。",  // DESLIZAMIENTO
  "じぶんが ひんしに なると あいても ひんしに する。",  // DESTINYBOND
  "こうげきを ふせぐ。 れんぞくで つかうと しっぱい しやすい。",  // DETECT
  "ダイヤの あらしで こうげき。 じぶんの ぼうぎょが あがる ことが ある。",  // DIAMONDSTORM
  "1ターンめに あなを ほり 2ターンめに こうげき。 ダンジョンから でる ことも できる。",  // DIG
  "きけんな ツメで きゅうしょを ねらう。 どく まひ ねむりに する ことが ある。",  // DIRECLAW
  "あいてが さいごに つかった わざを 4ターン つかえなくする。",  // DISABLE
  "みりょくてきな こえで あいてに ダメージ。 かならず あたる。",  // DISARMINGVOICE
  "でんきを まきちらし まわり すべてを こうげき。 まひ させる ことが ある。",  // DISCHARGE
  "1ターンめに もぐり 2ターンめに こうげきする。",  // DIVE
  "リズミカルな パンチ。 こんらん させる ことが ある。",  // DIZZYPUNCH
  "わざを つかった 2ターンご ひかりの たばで あいてを こうげきする。",  // DOOMDESIRE
  "いのちがけの たいあたり。 じぶんも ダメージを うける。",  // DOUBLEEDGE
  "しっぽや ツタで 2かい つづけて たたく。",  // DOUBLEHIT
  "こぶしで 2かい つづけて なぐる。 ひるませる ことが ある。",  // DOUBLEIRONBASH
  "2ほんの あしで けって 2かい こうげきする。",  // DOUBLEKICK
  "からだの でんきを すべて つかって こうげき。 でんきタイプで なくなる。",  // DOUBLESHOCK
  "おうふくビンタで 2から 5かい つづけて たたく。",  // DOUBLESLAP
  "ぶんしんを つくり かいひりつを あげる。",  // DOUBLETEAM
  "そらから りゅうせいを おとす。 じぶんの とくこうが がくっと さがる。",  // DRACOMETEOR
  "そらから すごい はやさで おちて こうげき。 じぶんの ぼうぎょと とくぼうが さがる。",  // DRAGONASCENT
  "すごい いきおいの いきで こうげき。 まひ させる ことが ある。",  // DRAGONBREATH
  "するどい ツメで あいてを ひっかく。",  // DRAGONCLAW
  "しんぴてきな まいで こうげきと すばやさを あげる。",  // DRAGONDANCE
  "ドラメシヤを とばして 2かい こうげき。 あいてが 2ひきなら 1かいずつ あてる。",  // DRAGONDARTS
  "いのちの ちからで こうげき。 じぶんの HPが すくないほど いりょくが さがる。",  // DRAGONENERGY
  "からだを ハンマーの ように して あいてに たたきつける。",  // DRAGONHAMMER
  "おおきな くちから しょうげきはを はなって こうげきする。",  // DRAGONPULSE
  "いかりの しょうげきはで かならず 40の ダメージを あたえる。",  // DRAGONRAGE
  "あいてを いあつしながら とっしん。 ひるませる ことが ある。",  // DRAGONRUSH
  "あいてを はじきとばし こうたい させる。 やせいでは せんとうが おわる。",  // DRAGONTAIL
  "キスで あいての HPを すいとり じぶんの HPを かいふくする。",  // DRAININGKISS
  "ちからを すいとる パンチ。 あたえた ダメージの はんぶんの HPを かいふく。",  // DRAINPUNCH
  "ねむっている あいての ゆめを くう。 あたえた ダメージの はんぶんを かいふく。",  // DREAMEATER
  "くちばしを ドリルの ように まわして つきさす。",  // DRILLPECK
  "ドリルの ように まわって たいあたり。 きゅうしょに あたりやすい。",  // DRILLRUN
  "ドラムを たたいて きりかぶを あやつり こうげき。 あいての すばやさを さげる。",  // DRUMBEATING
  "しっぽなど からだの いちぶで 2かい つづけて こうげきする。",  // DUALCHOP
  "つばさで 2かい つづけて あいてを うつ。",  // DUALWINGBEAT
  "コアから つよい エネルギーの ビームを うつ。",  // DYNAMAXCANNON
  "ちからを こめた パンチ。 あたれば かならず こんらん させる。",  // DYNAMICPUNCH
  "あいての あしもとの だいちを ふんか させる。 とくぼうを さげる ことが ある。",  // EARTHPOWER
  "じしんを おこして まわりの ポケモン すべてを こうげきする。",  // EARTHQUAKE
  "じぶんや ほかの ポケモンが つかうたびに いりょくが あがる こえ。",  // ECHOEDVOICE
  "ふしぎな でんぱを だして あいての とくこうを がくっと さげる。",  // EERIEIMPULSE
  "おおきな タマゴを ちからいっぱい なげつける。",  // EGGBOMB
  "5ターン エレキフィールドに する。 じめんに いる ポケモンは ねむらない。",  // ELECTRICTERRAIN
  "でんきの たまを ぶつける。 あいてより はやいほど ダメージが おおきい。",  // ELECTROBALL
  "へんけいしながら とびかかる。 こうかばつぐんだと いりょくが あがる。",  // ELECTRODRIFT
  "1ターンめに でんきを ためて とくこうを あげ 2ターンめに こうげき。 あめなら すぐ うつ。",  // ELECTROSHOT
  "でんきの あみで まわりの あいてを とらえ すばやさを さげる。",  // ELECTROWEB
  "ちいさな ほのおで こうげき。 やけどに する ことが ある。",  // EMBER
  "あいてに さいごの わざを 3ターン くりかえさせる。",  // ENCORE
  "あいての HPを じぶんの HPと おなじに する。",  // ENDEAVOR
  "どんな こうげきも HP1で こらえる。 れんぞくで つかうと しっぱい しやすい。",  // ENDURE
  "しぜんの ちからを あつめて はなつ。 とくぼうを さげる ことが ある。",  // ENERGYBALL
  "ふしぎな おどりで あいての とくせいを じぶんと おなじに する。",  // ENTRAINMENT
  "ばくはつてきな いかり。 じぶんの HPが すくないほど いりょくが さがる。",  // ERUPTION
  "すさまじい さむけを あたえて あいてを こおらせる。",  // ESCALOFRIO
  "オーラの つばさで きる。 きゅうしょに あたりやすく すばやさが あがる。",  // ESPERWING
  "げんしょの すがたの ムゲンダイナの さいきょうわざ。 つぎの ターン うごけない。",  // ETERNABEAM
  "サイコパワーで こうげき。 サイコフィールドでは いりょくが あがり あいて ぜんいんに あたる。",  // EXPANDINGFORCE
  "ばくはつして まわり すべてに ダメージ。 じぶんは すぐ ひんしに なる。",  // EXPLOSION
  "ふしぎな ちからで こうげき。 ひるませる ことが ある。",  // EXTRASENSORY
  "ものすごい はやさの こうげき。 かならず せんせい できる。",  // EXTREMESPEED
  "やけど まひ どくの とき いりょくが 2ばいに なる。",  // FACADE
  "ようせいの かぜを おこして あいてに ふきつける。",  // FAIRYWIND
  "せんせいで こうげきし かならず ひるませる。 さいしょの ターンしか きかない。",  // FAKEOUT
  "なきまねで ゆだん させ あいての とくぼうを がくっと さげる。",  // FAKETEARS
  "おじぎする ふりをして かみのけで つきさす。 かならず あたる。",  // FALSESURRENDER
  "てかげんした こうげき。 あいての HPを かならず 1 のこす。",  // FALSESWIPE
  "はねを まきちらし あいての こうげきを がくっと さげる。",  // FEATHERDANCE
  "まもるや みきりを つかう あいてにも あたり その こうかを けす。",  // FEINT
  "あいてを ゆだん させて なぐる。 かならず あたる。",  // FEINTATTACK
  "この わざで あいてを たおすと じぶんの こうげきが ぐぐーんと あがる。",  // FELLSTINGER
  "ときどき ほかの くびも こうげきに くわわり 2ばいの ダメージに なる。",  // FICKLEBEAM
  "ほのおを まとって おどる。 じぶんの とくこうが あがる ことが ある。",  // FIERYDANCE
  "いのちを かけて のこりの HPと おなじ ダメージを あたえ ひんしに なる。",  // FINALGAMBIT
  "はげしい ほのおで こうげき。 やけどに する ことが ある。",  // FIREBLAST
  "ほのおの キバで かみつく。 ひるませるか やけどに する ことが ある。",  // FIREFANG
  "もえる ムチで たたき あいての ぼうぎょを さげる。",  // FIRELASH
  "ほのおを まとった パンチ。 やけどに する ことが ある。",  // FIREPUNCH
  "ほのおの うずに 4から 5ターン とじこめる。",  // FIRESPIN
  "いりょくが たかい せんせいわざ。 でてきた さいしょの ターンしか つかえない。",  // FIRSTIMPRESSION
  "エラで かみつく。 あいてより さきに こうげき できれば いりょく 2ばい。",  // FISHIOUSREND
  "じわれに あいてを おとす。 あたれば いちげきで ひんしに する。",  // FISSURE
  "あばれて こうげき。 じぶんの HPが すくないほど ダメージが おおきい。",  // FLAIL
  "はじける ほのおで こうげき。 となりの ポケモンにも ダメージ。",  // FLAMEBURST
  "ほのおで こうげきして じぶんの すばやさを あげる。",  // FLAMECHARGE
  "はげしい ほのおで こうげき。 やけどに する ことが ある。",  // FLAMETHROWER
  "ほのおを まとって とっしん。 やけどに する ことが ある。",  // FLAMEWHEEL
  "ほのおを まとって とっしん。 じぶんも ダメージを うける。",  // FLAREBLITZ
  "まぶしい ひかりで あいての めいちゅうりつを さげる。",  // FLASH
  "からだの ひかりを あつめて はなつ。 とくぼうを さげる ことが ある。",  // FLASHCANNON
  "あいてを おだてて こんらん させるが とくこうも あげてしまう。",  // FLATTER
  "つよい ビームを はなつ。 じぶんの とくこうが がくっと さがる。",  // FLEURCANNON
  "もちものを なげつける。 どうぐで いりょくと こうかが かわる。",  // FLING
  "こうげきの あと すばやく てもちの ポケモンと いれかわる。",  // FLIPTURN
  "しかけの ある はなたばを なげる。 かならず あたり かならず きゅうしょに あたる。",  // FLOWERTRICK
  "1ターンめに そらへ とび 2ターンめに こうげきする。",  // FLY
  "あいてに とびかかる。 かくとうと ひこうの 2つの タイプを もつ わざ。",  // FLYINGPRESS
  "きあいを たかめて ちからを はなつ。 とくぼうを さげる ことが ある。",  // FOCUSBLAST
  "きあいを ためて きゅうしょに あたりやすくする。",  // FOCUSENERGY
  "きあいを ためて パンチ。 そのまえに こうげきを うけると しっぱいする。",  // FOCUSPUNCH
  "しょうげきはで こうげき。 まひ させる ことが ある。",  // FORCEPALM
  "ゴーストタイプにも わざが あたり かいひりつの たかい あいてにも あたるように なる。",  // FORESIGHT
  "あいての こうげきの ちからを つかって ダメージを あたえる。",  // FOULPLAY
  "いっきに ひやす。 こおらせる ことが あり みずタイプに ばつぐん。",  // FREEZEDRY
  "こおった きりの けっしょうで こうげき。 ぜんいんの のうりょくの へんかを けす。",  // FREEZYFROST
  "つめたい いきを ふきつける。 かならず きゅうしょに あたる。",  // FROSTBREATH
  "トレーナーへの なつきどが ひくいほど いりょくが あがる。",  // FRUSTRATION
  "つのや くちばしで 2から 5かい つづけて つく。",  // FURYATTACK
  "ツメや カマの こうげき。 つづけて つかうほど いりょくが あがる。",  // FURYCUTTER
  "ツメや カマで 2から 5かい すばやく ひっかく。",  // FURYSWIPES
  "おおきな でんげき。 おおきな ほのおの えいきょうを うけると いりょくが あがる。",  // FUSIONBOLT
  "おおきな ほのお。 おおきな でんきの えいきょうを うけると いりょくが あがる。",  // FUSIONFLARE
  "わざを つかった 2ターンご サイコパワーの かたまりが あいてを おそう。",  // FUTURESIGHT
  "いえきを はきかけて あいての とくせいを けす。",  // GASTROACID
  "てつの ギアを 2つ なげて 2かい こうげきする。",  // GEARGRIND
  "1ターンめに ちからを ため 2ターンめに とくこう とくぼう すばやさを ぐーんと あげる。",  // GEOMANCY
  "あたえた ダメージの はんぶんの HPを すいとる。",  // GIGADRAIN
  "ぜんりょくで とっしん。 つぎの ターンは うごけない。",  // GIGAIMPACT
  "おおきな ハンマーで なぐる。 2かい つづけて つかえない。",  // GIGATONHAMMER
  "こおった くうきを ふきつけ あいての すばやさを さげる。",  // GLACIATE
  "つぎの ターン あいての こうげきが かならず あたり 2ばいの ダメージを うける。",  // GLAIVERUSH
  "おそろしい めつきで にらみ あいてを まひ させる。",  // GLARE
  "ねんりきで こうげきし ひかりのかべを はる。",  // GLITZYGLOW
  "くさを からませて ころばせる。 あいてが おもいほど いりょくが あがる。",  // GRASSKNOT
  "ここちよい ねいろで あいてを ふかく ねむらせる。",  // GRASSWHISTLE
  "じめんを すべって こうげき。 グラスフィールドでは せんせい できる。",  // GRASSYGLIDE
  "5ターン グラスフィールドに する。 ターンの おわりに じめんの ポケモンの HPが かいふく。",  // GRASSYTERRAIN
  "たかい ところから りんごを おとす。 あいての ぼうぎょを さげる。",  // GRAVAPPLE
  "かわいく なきごえを あげて あいての こうげきを さげる。",  // GROWL
  "からだを いっきに そだてて こうげきと とくこうを あげる。",  // GROWTH
  "こうげきで ひんしに なると その わざの PPを 0に する。",  // GRUDGE
  "おおきな ハサミで はさむ。 あたれば いちげきで ひんしに する。",  // GUILLOTINE
  "きたない ゴミを ぶつける。 どくに する ことが ある。",  // GUNKSHOT
  "つばさで たつまきを おこして あいてに ぶつける。",  // GUST
  "かいてんして たいあたり。 じぶんが おそいほど ダメージが おおきい。",  // GYROBALL
  "おもい こぶしを ふりおろす。 じぶんの すばやさが さがる。",  // HAMMERARM
  "からだに ちからを いれて ぼうぎょを あげる。",  // HARDEN
  "くろい きりで ぜんいんの のうりょくの へんかを もとに もどす。",  // HAZE
  "あたまから つっこむ。 ひるませる ことが ある。",  // HEADBUTT
  "あたまで つよく たいあたり。 じぶんも すこし ダメージを うける。",  // HEADCHARGE
  "ぜんりょくで ぶつかる。 じぶんの ぼうぎょと とくぼうが さがる。",  // HEADLONGRUSH
  "いのちを かけた ずつき。 じぶんも おおきな ダメージを うける。",  // HEADSMASH
  "やさしい すずの ねいろで みかた ぜんいんの じょうたいいじょうを なおす。",  // HEALBELL
  "じぶんは ひんしに なるが かわりの ポケモンの HPと じょうたいを なおす。",  // HEALINGWISH
  "なかまを よんで さいだいHPの はんぶんまで かいふくする。",  // HEALORDER
  "いやしの はどうで あいての HPを さいだいHPの はんぶん かいふくする。",  // HEALPULSE
  "かわいい しぐさで すきを つき つよく うつ。 ひるませる ことが ある。",  // HEARTSTAMP
  "じぶんが あいてより おもいほど いりょくが あがる。",  // HEATCRASH
  "あつい かぜで こうげき。 やけどに する ことが ある。",  // HEATWAVE
  "じぶんが あいてより おもいほど いりょくが あがる。",  // HEAVYSLAM
  "じょうたいいじょうの あいてに おおきな ダメージを あたえる。",  // HEX
  "つかう ポケモンで タイプと いりょくが かわる わざ。",  // HIDDENPOWER
  "からだ ぜんたいで はげしく こうげきする。",  // HIGHHORSEPOWER
  "たかく とんで ひざげり。 はずれると じぶんが ダメージを うける。",  // HIGHJUMPKICK
  "ツメを といで こうげきと めいちゅうりつを あげる。",  // HONECLAWS
  "とがった ツノで ついて こうげきする。",  // HORNATTACK
  "まわる ツノで つきさす。 あたれば いちげきで ひんしに する。",  // HORNDRILL
  "ちからを すいとる こうげき。 あたえた ダメージの はんぶんの HPを かいふく。",  // HORNLEECH
  "げんきよく ほえて こうげきを あげる。",  // HOWL
  "はげしい ぼうふうで つつみこむ。 こんらん させる ことが ある。",  // HURRICANE
  "たくさんの みずを いきおいよく ふきつける。",  // HYDROPUMP
  "あつい ねっとうを ふきつける。 ひざしが つよいと いりょくが あがる。",  // HYDROSTEAM
  "すさまじい ビーム。 つぎの ターンは うごけない。",  // HYPERBEAM
  "からだの とがった ぶぶんを まわして こうげき。 まもるなどを むしする。",  // HYPERDRILL
  "するどい まえばで かみつく。 ひるませる ことが ある。",  // HYPERFANG
  "まもる みきり ワイドガード ファストガードを つかう あいてにも あたる。",  // HYPERSPACEHOLE
  "けたたましい さけびごえで あいてに ダメージを あたえる。",  // HYPERVOICE
  "さいみんじゅつで あいてを ふかい ねむりに さそう。",  // HYPNOSIS
  "5ターン ころがって こうげき。 あたるたびに いりょくが あがる。",  // ICEBALL
  "こおりの ビーム。 こおらせる ことが ある。",  // ICEBEAM
  "こおりの キバで かみつく。 ひるませるか こおらせる ことが ある。",  // ICEFANG
  "おもい こぶしで なぐる。 じぶんの すばやさが さがる。",  // ICEHAMMER
  "こおりの パンチ。 こおらせる ことが ある。",  // ICEPUNCH
  "こおりの つぶてを すばやく なげる。 かならず せんせい できる。",  // ICESHARD
  "あしに こおりを まとって こうげき。 フィールドが あれば こわす。",  // ICESPINNER
  "おおきな つららを ぶつける。 ひるませる ことが ある。",  // ICICLECRASH
  "するどい つららを 2から 5かい つづけて うちだす。",  // ICICLESPEAR
  "こおった かぜで こうげきし あいての すばやさを さげる。",  // ICYWIND
  "じぶんが おぼえている わざを あいてが つかえなくなる。",  // IMPRISON
  "ほのおで まわりの あいてを こうげき。 きのみを もっていれば もやして なくす。",  // INCINERATE
  "はげしい ほのおで こうげきし かならず やけどに する。",  // INFERNO
  "あいてに 4から 5ターン まとわりつく。 そのあいだ あいては にげられない。",  // INFESTATION
  "ねを はって まいターン HPを かいふく。 いれかえが できなくなる。",  // INGRAIN
  "でんきを おびた つぶを まき ノーマルわざを でんきタイプに する。",  // IONDELUGE
  "からだを てつの ように かたくして ぼうぎょを ぐーんと あげる。",  // IRONDEFENSE
  "かたい てつの あたまで こうげき。 ひるませる ことが ある。",  // IRONHEAD
  "かたい しっぽで たたく。 ぼうぎょを さげる ことが ある。",  // IRONTAIL
  "こんぼうで なぐる。 めんで タイプが かわり きゅうしょに あたりやすい。",  // IVYCUDGEL
  "どちらかが ひんしに なるまで おたがいに にげられなくなる。",  // JAWLOCK
  "みずの うずを まとった こぶしで せんせいこうげき する。",  // JETPUNCH
  "むすうの ひかりを はなつ。 もっている プレートで タイプが かわる。",  // JUDGMENT
  "とびあがって けりを はなつ。 はずれると じぶんが ダメージを うける。",  // JUMPKICK
  "じぶんと みかたの HPを かいふくし じょうたいいじょうも なおす。",  // JUNGLEHEALING
  "するどい チョップで こうげき。 きゅうしょに あたりやすい。",  // KARATECHOP
  "スプーンを まげて きを ひき あいての めいちゅうりつを さげる。",  // KINESIS
  "こうげきを ふせぐ。 ふれた あいての こうげきを がくっと さげる。",  // KINGSSHIELD
  "あいての もちものを はたいて せんとうちゅう つかえなくする。",  // KNOCKOFF
  "どげざして ゆだん させてから きりつける。 かならず あたる。",  // KOWTOWCLEAVE
  "だいちの ちからを あつめて あいてに ぶつけ ダメージを あたえる。",  // LANDSWRATH
  "しゅうちゅうして つぎの こうげきを かならず きゅうしょに あてる。",  // LASERFOCUS
  "ほかの わざを ぜんぶ つかった あとで ないと だせない。",  // LASTRESORT
  "たおれた なかまの むねんを はらす。 ひんしの なかまが おおいほど いりょくが あがる。",  // LASTRESPECTS
  "はげしい ほのおで まわりの ポケモンを こうげき。 やけどに する ことが ある。",  // LAVAPLUME
  "はっぱを ぶつけて あいてを こうげきする。",  // LEAFAGE
  "するどい はっぱで きりつける。 きゅうしょに あたりやすい。",  // LEAFBLADE
  "するどい はっぱの あらし。 じぶんの とくこうが がくっと さがる。",  // LEAFSTORM
  "するどい はっぱで まきこむ。 めいちゅうりつを さげる ことが ある。",  // LEAFTORNADO
  "ちを すって あたえた ダメージの はんぶんの HPを かいふくする。",  // LEECHLIFE
  "タネを うえつけ まいターン あいての HPを すいとって かいふくする。",  // LEECHSEED
  "するどい めつきで にらみ あいての ぼうぎょを さげる。",  // LEER
  "ながい したで なめる。 まひ させる ことが ある。",  // LICK
  "ふしぎな みずを ふりまいて じぶんと みかたの HPを かいふくする。",  // LIFEDEW
  "ひかりの かべで 5ターン とくしゅわざの ダメージを へらす。",  // LIGHTSCREEN
  "みずの ちからで たたく。 ぼうぎょを さげる ことが ある。",  // LIQUIDATION
  "ねらいを さだめて つぎの わざを かならず あてる。",  // LOCKON
  "おそろしい かおで キスをして あいてを ねむらせる。",  // LOVELYKISS
  "あしを けって たおす。 あいてが おもいほど ダメージが おおきい。",  // LOWKICK
  "すばやく あしもとを ねらい あいての すばやさを さげる。",  // LOWSWEEP
  "そらに いのり あいての こうげきが きゅうしょに あたらなくなる。",  // LUCKYCHANT
  "こころに ひびく ふしぎな ひかりで こうげき。 とくぼうを がくっと さげる。",  // LUMINACRASH
  "つきに いのり みかたの HPを かいふくし じょうたいいじょうを なおす。",  // LUNARBLESSING
  "じぶんは ひんしに なるが かわりの ポケモンの HP PP じょうたいを なおす。",  // LUNARDANCE
  "ぜんりょくで とびかかり あいての こうげきを さげる。",  // LUNGE
  "まぶしい ひかりを はなつ。 とくぼうを さげる ことが ある。",  // LUSTERPURGE
  "めにも とまらぬ パンチ。 かならず せんせい できる。",  // MACHPUNCH
  "ふしぎな はっぱが あいてを おいかける。 かならず あたる。",  // MAGICALLEAF
  "ほのおの うずに 2から 5ターン とじこめる。",  // MAGMASTORM
  "くっつく てつの ばくだんを なげる。 かならず あたる。",  // MAGNETBOMB
  "でんきの じりょくで 5ターン ちゅうに うく。",  // MAGNETRISE
  "まわりの ポケモン すべてを ゆらす じしん。 いりょくが かわる。",  // MAGNITUDE
  "たくさんの コインを なげる。 とくこうが さがり もらえる おかねが ふえる。",  // MAKEITRAIN
  "どくの くさりで しめつける。 もうどくに する ことが ある。",  // MALIGNANTCHAIN
  "おちゃを ふきつけ ダメージの はんぶんを かいふく。 やけどに する ことが ある。",  // MATCHAGOTCHA
  "くろい まなざしで にらみ あいてを にげられなくする。",  // MEANLOOK
  "めいそうして ちからを めざめさせ こうげきを あげる。",  // MEDITATE
  "あたえた ダメージの はんぶんの HPを すいとる。",  // MEGADRAIN
  "りっぱな ツノで はげしく つっこむ。",  // MEGAHORN
  "ものすごい ちからの キックで こうげきする。",  // MEGAKICK
  "ちからを こめた パンチで こうげきする。",  // MEGAPUNCH
  "じぶんは ひんしに なるが あいての こうげきと とくこうを がくっと さげる。",  // MEMENTO
  "さいごに うけた こうげきを もっと つよくして かえす。",  // METALBURST
  "はがねの ツメで ひっかく。 じぶんの こうげきが あがる ことが ある。",  // METALCLAW
  "きんぞくの いやな おとで あいての とくぼうを がくっと さげる。",  // METALSOUND
  "ながねぎで うちかかる。 つぎの ターンは うごけない。",  // METEORASSAULT
  "1ターンめに とくこうを あげ 2ターンめに こうげきする。",  // METEORBEAM
  "りゅうせいの ような パンチ。 じぶんの こうげきが あがる ことが ある。",  // METEORMASH
  "あたまに ためた ひかりで きる。 まもっている あいてにも あたる。",  // MIGHTYCLEAVE
  "さいだいHPの はんぶんを かいふく。 せんとうの そとでは なかまに HPを わけられる。",  // MILKDRINK
  "あたまを ばくはつ させて まわりを こうげき。 じぶんも ダメージを うける。",  // MINDBLOWN
  "あいての うごきを よみ つぎの こうげきを かならず あてる。",  // MINDREADER
  "からだを ちぢめて かいひりつを ぐーんと あげる。",  // MINIMIZE
  "あくタイプにも わざが あたり かいひりつの たかい あいてにも あたるように なる。",  // MIRACLEEYE
  "とくしゅわざを うけると 2ばいの ダメージで かえす。",  // MIRRORCOAT
  "あいてが さいごに つかった わざを まねして かえす。",  // MIRRORMOVE
  "みがいた からだから ひかりを はなつ。 めいちゅうりつを さげる ことが ある。",  // MIRRORSHOT
  "しろい きりで 5ターン みかたの のうりょくを さげられなくする。",  // MIST
  "きりの ような はねで こうげき。 とくこうを さげる ことが ある。",  // MISTBALL
  "ふしぎな きりで こうげき。 ミストフィールドでは せんせい できる。",  // MISTYEXPLOSION
  "5ターン ミストフィールドに する。 じめんの ポケモンは じょうたいいじょうに ならない。",  // MISTYTERRAIN
  "つきの ちからで こうげき。 とくこうを さげる ことが ある。",  // MOONBLAST
  "ふしぎな つきの ひかりで こうげき。 あいての とくせいを むしする。",  // MOONGEISTBEAM
  "じぶんの HPを かいふく。 てんきで かいふくする りょうが かわる。",  // MOONLIGHT
  "じぶんの HPを かいふく。 てんきで かいふくする りょうが かわる。",  // MORNINGSUN
  "かいてんして あいてを どくに し しめつけや やどりぎのタネを はずす。",  // MORTALSPIN
  "ひょうざんの ような つららを ぶつける。 ひるませる ことが ある。",  // MOUNTAINGALE
  "かたい どろの たまを ぶつける。 めいちゅうりつを さげる ことが ある。",  // MUDBOMB
  "どろみずで こうげき。 めいちゅうりつを さげる ことが ある。",  // MUDDYWATER
  "どろを ぶつけて あいての すばやさを さげる。",  // MUDSHOT
  "かおに どろを かけて めいちゅうりつを さげる。",  // MUDSLAP
  "どろを かぶって ばに いるあいだ でんきわざの いりょくを よわめる。",  // MUDSPORT
  "つよい エネルギーで こうげき。 もっている メモリで タイプが かわる。",  // MULTIATTACK
  "あつい ほのおを ふきかけ あいての とくこうを さげる。",  // MYSTICALFIRE
  "ふしぎな ちからで こうげきし じぶんの とくこうを あげる。",  // MYSTICALPOWER
  "わるい ことを かんがえて とくこうを ぐーんと あげる。",  // NASTYPLOT
  "もっている きのみの ちからで こうげき。 きのみで タイプと いりょくが かわる。",  // NATURALGIFT
  "しぜんの いかりで あいての HPを はんぶんに する。",  // NATURESMADNESS
  "トゲの ある うでで なぐる。 ひるませる ことが ある。",  // NEEDLEARM
  "まっくろな はどうを はなつ。 めいちゅうりつを さげる ことが ある。",  // NIGHTDAZE
  "まぼろしを みせて じぶんの レベルと おなじ ダメージを あたえる。",  // NIGHTSHADE
  "すきを ついて すばやく きる。 きゅうしょに あたりやすい。",  // NIGHTSLASH
  "おたけびで いかくして あいての こうげきと とくこうを さげる。",  // NOBLEROAR
  "じぶんの のうりょくを ぜんぶ あげるが にげられず こうたいも できない。",  // NORETREAT
  "でんきの ほっぺを すりつけて あいてを まひ させる。",  // NUZZLE
  "あいての ちからを すいとり あたえた ダメージの 3/4を かいふくする。",  // OBLIVIONWING
  "こうげきを ふせぐ。 ふれた あいての ぼうぎょを がくっと さげる。 れんぞくでは しっぱい。",  // OBSTRUCT
  "かおに スミを ふきかける。 めいちゅうりつを さげる ことが ある。",  // OCTAZOOKA
  "あいてを とらえて にげられなくし まいターン ぼうぎょと とくぼうを さげる。",  // OCTOLOCK
  "ゴーストタイプにも わざが あたり かいひりつの たかい あいてにも あたるように なる。",  // ODORSLEUTH
  "あやしい かぜで こうげき。 じぶんの のうりょくが ぜんぶ あがる ことが ある。",  // OMINOUSWIND
  "くちに シャリタツが いると その すがたに おうじて のうりょくを あげる。",  // ORDERUP
  "むすうの あおい ひかりで あいてを こうげきする。",  // ORIGINPULSE
  "2から 3ターン あばれて こうげき。 そのあと じぶんが こんらんする。",  // OUTRAGE
  "からだを かきならし おおきな しんどうで こうげきする。",  // OVERDRIVE
  "ぜんりょくの こうげき。 じぶんの とくこうが がくっと さがる。",  // OVERHEAT
  "じぶんと あいての HPを たして おなじに わけあう。",  // PAINSPLIT
  "まわりの ポケモンを こうげきし ダメージの はんぶんの HPを かいふく。",  // PARABOLICCHARGE
  "あいての こうげきと とくこうを さげて てもちの ポケモンと いれかわる。",  // PARTINGSHOT
  "あいてが さきに わざを つかうと いりょく 2ばいで かえす。",  // PAYBACK
  "コインを なげつける。 せんとうの あとで おかねが もらえる。",  // PAYDAY
  "するどい ツノや くちばしで つつく。",  // PECK
  "はげしい はなびらの あらしで まわりの ポケモン すべてを こうげき。",  // PETALBLIZZARD
  "2から 3ターン はなびらを まいちらす。 そのあと じぶんが こんらんする。",  // PETALDANCE
  "1ターンめに きえて 2ターンめに こうげき。 まもっている あいてにも あたる。",  // PHANTOMFORCE
  "こうげきと とくこうの たかい ほうで ダメージを あたえる。",  // PHOTONGEYSER
  "するどい はりを 2から 5かい つづけて うちだす。",  // PINMISSILE
  "こぶしで こうげき。 ノーマルわざを でんきタイプに する。",  // PLASMAFISTS
  "なかよくして たたかう きを なくさせ あいての こうげきを さげる。",  // PLAYNICE
  "じゃれついて こうげき。 あいての こうげきを さげる ことが ある。",  // PLAYROUGH
  "くちばしで つつく。 あいてが きのみを もっていれば たべて こうかを えられる。",  // PLUCK
  "どくの キバで かみつく。 もうどくに する ことが ある。",  // POISONFANG
  "どくガスを あいてに ふきかけて どくに する。",  // POISONGAS
  "どくの しょくしゅや うでで つきさす。 どくに する ことが ある。",  // POISONJAB
  "どくの こなを まいて あいてを どくに する。",  // POISONPOWDER
  "どくの はりで さす。 どくに する ことが ある。",  // POISONSTING
  "しっぽで こうげき。 どくに する ことが あり きゅうしょに あたりやすい。",  // POISONTAIL
  "ばくはつする だんごで こうげき。 みかたに あてると HPを かいふくする。",  // POLLENPUFF
  "あいての もちもので こうげきする。 もちものが ないと しっぱいする。",  // POLTERGEIST
  "なかまと いっしょに 1から 10かい つづけて こうげきする。",  // POPULATIONBOMB
  "あいてに とびついて こうげきし すばやさを さげる。",  // POUNCE
  "しっぽや てあしで あいてを たたく。",  // POUND
  "つめたい こなゆきで こうげき。 こおらせる ことが ある。",  // POWDERSNOW
  "たくさんの ほうせきの ように かがやく ひかりで こうげきする。",  // POWERGEM
  "じぶんの のうりょくが あがっているほど いりょくが あがる。",  // POWERTRIP
  "なぐるほど こぶしが かたくなる。 あたると じぶんの こうげきが あがる。",  // POWERUPPUNCH
  "ツタや しょくしゅを はげしく ふりまわして たたきつける。",  // POWERWHIP
  "だいちの ちからを するどい やいばに かえて こうげきする。",  // PRECIPICEBLADES
  "しかけの ある プレゼントを わたす。 ときどき HPを かいふくして しまう。",  // PRESENT
  "プリズムから つよい ビームを うつ。 つぎの ターンは うごけない。",  // PRISMATICLASER
  "こうげきを ふせぐ。 れんぞくで つかうと しっぱい しやすい。",  // PROTECT
  "ふしぎな こうせんで こうげき。 こんらん させる ことが ある。",  // PSYBEAM
  "みえない つるぎで きりさく。 エレキフィールドでは いりょくが あがる。",  // PSYBLADE
  "つよい ねんりきで こうげき。 とくぼうを さげる ことが ある。",  // PSYCHIC
  "サイコパワーで かみつき ひかりのかべや リフレクターも こわす。",  // PSYCHICFANGS
  "5ターン エスパーわざが つよくなり せんせいわざが しっぱいする。",  // PSYCHICTERRAIN
  "ぜんりょくの こうげき。 じぶんの とくこうが がくっと さがる。",  // PSYCHOBOOST
  "サイコパワーの やいばで きりさく。 きゅうしょに あたりやすい。",  // PSYCHOCUT
  "じこあんじで あいての のうりょくの へんかを じぶんに うつす。",  // PSYCHUP
  "サイコパワーを まとって こうげき。 じぶんの ぼうぎょも あがる。",  // PSYSHIELDBASH
  "サイコパワーの はどうで おおきな ぶつりてきな ダメージを あたえる。",  // PSYSHOCK
  "ふしぎな はどうで ぶつりてきな ダメージを あたえる。",  // PSYSTRIKE
  "つよさが かわる ふしぎな はどうで こうげきする。",  // PSYWAVE
  "あいての のうりょくが あがっているほど いりょくが あがる。",  // PUNISHMENT
  "あいての じょうたいいじょうを なおし じぶんの HPを かいふくする。",  // PURIFY
  "こうたい する あいてには 2ばいの ダメージを あたえる。",  // PURSUIT
  "ちいさな いしに ひを つけて ぶつける。 やけどに する ことが ある。",  // PYROBALL
  "めにも とまらぬ はやさで とっしん。 かならず せんせい できる。",  // QUICKATTACK
  "みかたを せんせいわざから まもる。 れんぞくで つかうと しっぱい しやすい。",  // QUICKGUARD
  "しんぴてきな まいで とくこう とくぼう すばやさを あげる。",  // QUIVERDANCE
  "つかっている あいだ こうげきを うけるたびに こうげきが あがる。",  // RAGE
  "いかりを ちからに かえる。 こうげきを うけた かずが おおいほど いりょくが あがる。",  // RAGEFIST
  "すがたで タイプが かわる。 かべを こわす。",  // RAGINGBULL
  "2から 3ターン はげしい ほのおで あばれ そのあと こんらんする。",  // RAGINGFURY
  "5ターン あめを ふらせて みずわざを つよくする。",  // RAINDANCE
  "かいてんして すばやさを あげ しめつけや やどりぎのタネ まきびしを はずす。",  // RAPIDSPIN
  "するどい はっぱで きりつける。 きゅうしょに あたりやすい。",  // RAZORLEAF
  "するどい かいがらで きる。 ぼうぎょを さげる ことが ある。",  // RAZORSHELL
  "かぜの やいばで きりつける。 きゅうしょに あたりやすい。",  // RAZORWIND
  "さいだいHPの はんぶんまで HPを かいふくする。",  // RECOVER
  "せんとうで つかった 1かいきりの もちものを もういちど つかえるように する。",  // RECYCLE
  "ひかりの かべで 5ターン ぶつりわざの ダメージを へらす。",  // REFLECT
  "からだを やすめて まひ どく やけどを なおす。",  // REFRESH
  "いにしえの うたで まわりの あいてを こうげき。 ねむらせる ことが ある。",  // RELICSONG
  "HPと じょうたいいじょうを なおす。 つぎの 2ターン ねむる。",  // REST
  "まえの ターンに なかまが たおされていると いりょくが あがる。",  // RETALIATE
  "トレーナーへの なつきどが たかいほど いりょくが あがる。",  // RETURN
  "はげしく おどる。 じぶんの 1つめの タイプに なり なければ 2つめの タイプに なる。",  // REVELATIONDANCE
  "そのターン あいてに こうげき されていると 2ばいの ダメージを あたえる。",  // REVENGE
  "じぶんの HPが すくないほど いりょくが あがる。",  // REVERSAL
  "あいてが エレキフィールドに いると いりょくが 2ばいに なる。",  // RISINGVOLTAGE
  "おどかして あいてを こうたい させる。 やせいでは せんとうが おわる。",  // ROAR
  "じかんを ゆがめる ほどの ちからで こうげき。 つぎの ターンは うごけない。",  // ROAROFTIME
  "かたい いわを 2から 5かい つづけて なげつける。",  // ROCKBLAST
  "はげしく とっしんする。 こんらん させる ことが ある。",  // ROCKCLIMB
  "からだを みがいて くうきの ていこうを へらし すばやさを ぐーんと あげる。",  // ROCKPOLISH
  "おおきな いわを なげつける。 ひるませる ことが ある。",  // ROCKSLIDE
  "いわを くだく パンチ。 ぼうぎょを さげる ことが ある。",  // ROCKSMASH
  "ちいさな いわを もちあげて なげつける。",  // ROCKTHROW
  "いわを なげて うごきを ふうじ あいての すばやさを さげる。",  // ROCKTOMB
  "おおきな いわを うちこむ。 つぎの ターンは うごけない。",  // ROCKWRECKER
  "すばやく まわって けりを はなつ。 ひるませる ことが ある。",  // ROLLINGKICK
  "からだを まるめて タイヤの ように ころがって こうげきする。",  // ROLLOUT
  "じめんに おりて やすむ。 さいだいHPの はんぶんまで かいふくする。",  // ROOST
  "うたって こうげき。 つかう ポケモンが おおいほど いりょくが あがる。",  // ROUND
  "すさまじい わざわいを おこし あいての HPを はんぶんに する。",  // RUINATION
  "しんぴてきな はげしい ほのお。 やけどに する ことが ある。",  // SACREDFIRE
  "つるぎで こうげき。 あいての のうりょくの へんかを むしする。",  // SACREDSWORD
  "5ターン みかたを じょうたいいじょうから まもる。",  // SAFEGUARD
  "あいては まいターン HPが へる。 はがねや みずタイプには よく きく。",  // SALTCURE
  "かおに すなを かけて めいちゅうりつを さげる。",  // SANDATTACK
  "あつい すなで つつみこむ。 やけどに する ことが ある。",  // SANDSEARSTORM
  "5ターン すなあらしを おこす。 いわ じめん はがねタイプ いがいに ダメージ。",  // SANDSTORM
  "すなの うずに 4から 5ターン とじこめる。",  // SANDTOMB
  "タネを まいて こうげきし やどりぎのタネを うえつける。",  // SAPPYSEED
  "ねっとうを ふきかける。 やけどに する ことが ある。",  // SCALD
  "うろこを 2から 5かい うつ。 すばやさが あがり ぼうぎょが さがる。",  // SCALESHOT
  "こわい かおで にらみ あいての すばやさを がくっと さげる。",  // SCARYFACE
  "あつい すなを なげつける。 やけどに する ことが ある。",  // SCORCHINGSANDS
  "するどい ツメで ひっかいて こうげきする。",  // SCRATCH
  "いやな おとで あいての ぼうぎょを がくっと さげる。",  // SCREECH
  "ほのおで まわりの ポケモンを こうげき。 やけどに する ことが ある。",  // SEARINGSHOT
  "ひみつの ちからで こうげき。 ばしょで ついかこうかが かわる。",  // SECRETPOWER
  "ふしぎな ちからを もつ つるぎで つきさす。",  // SECRETSWORD
  "ばくはつする タネを うえから ふらせて こうげきする。",  // SEEDBOMB
  "からだから しょうげきはを だす。 とくぼうを さげる ことが ある。",  // SEEDFLARE
  "じゅうりょくで なげとばし じぶんの レベルと おなじ ダメージを あたえる。",  // SEISMICTOSS
  "ばくはつして まわり すべてに ダメージ。 じぶんは すぐ ひんしに なる。",  // SELFDESTRUCT
  "くろい かげの たまを うつ。 とくぼうを さげる ことが ある。",  // SHADOWBALL
  "たましいの やどった ホネで なぐる。 ぼうぎょを さげる ことが ある。",  // SHADOWBONE
  "かげの ツメで きりさく。 きゅうしょに あたりやすい。",  // SHADOWCLAW
  "1ターンめに きえて 2ターンめに こうげき。 まもっている あいてにも あたる。",  // SHADOWFORCE
  "かげから くりだす パンチ。 かならず あたる。",  // SHADOWPUNCH
  "かげを のばして うしろから こうげき。 かならず せんせい できる。",  // SHADOWSNEAK
  "からだの かどを するどくして こうげきを あげる。",  // SHARPEN
  "ぜったいれいどの れいきで こうげき。 あたれば いちげきで ひんしに する。",  // SHEERCOLD
  "からを やぶり ぼうぎょと とくぼうが さがるが こうげき とくこう すばやさを ぐーんと あげる。",  // SHELLSMASH
  "ぶつりわざを うけると からの ワナが ばくはつして あいてに ダメージ。",  // SHELLTRAP
  "すばやい でんげき。 かならず あたる。",  // SHOCKWAVE
  "さいだいHPの はんぶんを かいふく。 すなあらしでは もっと かいふくする。",  // SHOREUP
  "あやしい ひかりの ビーム。 こんらん させる ことが ある。",  // SIGNALBEAM
  "いとの ワナで まもる。 ふれた あいての すばやさを さげる。",  // SILKTRAP
  "りんぷんを のせた かぜ。 じぶんの のうりょくが ぜんぶ あがる ことが ある。",  // SILVERWIND
  "やさしい こもりうたで あいてを ふかい ねむりに さそう。",  // SING
  "ほのおを まとって とっしん。 かならず やけどに する。",  // SIZZLYSLIDE
  "あいてが さいごに つかった わざを ずっと おぼえる。 1かい つかうと きえる。",  // SKETCH
  "うしろから こっそり こうげきして あいての とくこうを さげる。",  // SKITTERSMACK
  "1ターンめに くびを ひっこめて ぼうぎょを あげ 2ターンめに ずつき。",  // SKULLBASH
  "2ターンで だす わざ。 きゅうしょに あたりやすく ひるませる ことが ある。",  // SKYATTACK
  "いきおいよく アッパーを つきあげる。",  // SKYUPPERCUT
  "なまけて さいだいHPの はんぶんまで かいふくする。",  // SLACKOFF
  "しっぽや ツタで たたきつける。",  // SLAM
  "ツメや ハサミで きりさく。 きゅうしょに あたりやすい。",  // SLASH
  "ねむくなる こなを まいて あいてを ねむらせる。",  // SLEEPPOWDER
  "ヘドロを なげつける。 どくに する ことが ある。",  // SLUDGE
  "ヘドロを なげつける。 どくに する ことが ある。",  // SLUDGEBOMB
  "ヘドロの なみで まわりを こうげき。 どくに する ことが ある。",  // SLUDGEWAVE
  "いしなどを なげつける。 とんでいる あいてを じめんに おとす。",  // SMACKDOWN
  "するどい ツノで つきさす。 かならず あたる。",  // SMARTSTRIKE
  "まひの あいてに 2ばいの ダメージ。 そのかわり まひが なおる。",  // SMELLINGSALTS
  "きたない ガスで こうげき。 どくに する ことが ある。",  // SMOG
  "えんまくや スミで あいての めいちゅうりつを さげる。",  // SMOKESCREEN
  "トラバサミで 4から 5ターン はさみこむ。",  // SNAPTRAP
  "たえられない なきごえで こうげきし あいての とくこうを さげる。",  // SNARL
  "こうげきを ひきよせる とくせいや わざを むしして ねらいうつ。",  // SNIPESHOT
  "ねむっている ときに いびきで こうげき。 ひるませる ことが ある。",  // SNORE
  "5ターン ゆきを ふらせる。 こおりタイプの ぼうぎょが あがる。",  // SNOWSCAPE
  "たくさんの みずを かけて あいてを みずタイプに する。",  // SOAK
  "さいだいHPの はんぶんを かいふく。 せんとうの そとでは なかまに HPを わけられる。",  // SOFTBOILED
  "1ターンめに ひかりを あつめ 2ターンめに こうげきする。",  // SOLARBEAM
  "1ターンめに ひかりを ため 2ターンめに ひかりの やいばで きる。",  // SOLARBLADE
  "しょうげきはで かならず 20の ダメージを あたえる。",  // SONICBOOM
  "あいてを まわりの くうかんごと きりさく。 きゅうしょに あたりやすい。",  // SPACIALREND
  "でんきを まとって たいあたり。 まひ させる ことが ある。",  // SPARK
  "うたいながら あわを だす。 あわに あたった ポケモンの やけどを なおす。",  // SPARKLINGARIA
  "うずまきで つつんで こうげき。 みかたの じょうたいいじょうを なおす。",  // SPARKLYSWIRL
  "あいての かげに もぐり のうりょくの アップを うばってから こうげき。",  // SPECTRALTHIEF
  "ねばる いとで からめとり あいてを にげられなくする。",  // SPIDERWEB
  "するどい トゲを 2から 5かい つづけて うちだす。",  // SPIKECANNON
  "あいての ばに まきびしを まく。 でてきた あいてに ダメージを あたえる。",  // SPIKES
  "こうげきを ふせぎ ふれた あいてに ダメージを あたえる。",  // SPIKYSHIELD
  "はげしく かいてんする。 じぶんの すばやさが がくっと さがる。",  // SPINOUT
  "いきおいで せめて せんいを くじき あいての とくこうを さげる。",  // SPIRITBREAK
  "こうげきと どうじに かげを ぬいつけ あいてを にげられなくする。",  // SPIRITSHACKLE
  "うらみで あいてが さいごに つかった わざの PPを 4 へらす。",  // SPITE
  "たくわえるで ためた ちからを いっきに はきだす。",  // SPITUP
  "はねるだけで なにも おこらない。",  // SPLASH
  "ねむりを さそう ほうしを まきちらして あいてを ねむらせる。",  // SPORE
  "あいと にくしみの あらしで つつみこむ。 こうげきを さげる ことが ある。",  // SPRINGTIDESTORM
  "ういている いわの ワナを しかけ でてきた あいてに ダメージを あたえる。",  // STEALTHROCK
  "あつい じょうきで つつみこむ。 やけどに する ことが ある。",  // STEAMERUPTION
  "からだを まるめて ころがり おしつぶす。 ひるませる ことが ある。",  // STEAMROLLER
  "からだの はがねを あつめて ビームを うつ。 じぶんも ダメージを うける。",  // STEELBEAM
  "こうげきして フィールドを こわす。 フィールドが ないと しっぱいする。",  // STEELROLLER
  "かたい つばさで うつ。 じぶんの ぼうぎょが あがる ことが ある。",  // STEELWING
  "あいての ばに ネットを はり でてきた ポケモンの すばやさを さげる。",  // STICKYWEB
  "ちからを たくわえて ぼうぎょと とくぼうを あげる。 3かいまで つかえる。",  // STOCKPILE
  "おおきな あしで ふみつける。 ひるませる ことが ある。",  // STOMP
  "くやしさで こうげき。 まえの わざが しっぱい していると いりょく 2ばい。",  // STOMPINGTANTRUM
  "いしの オノで こうげき。 とびちった いしが あいての まわりに のこる。",  // STONEAXE
  "するどい いしを つきさす。 きゅうしょに あたりやすい。",  // STONEEDGE
  "じぶんの のうりょくが あがっているほど ダメージが おおきい。",  // STOREDPOWER
  "はげしい いちげきを くりだし かならず きゅうしょに あたる。",  // STORMTHROW
  "じょうきを ふきだして こうげき。 こんらん させる ことが ある。",  // STRANGESTEAM
  "ちからいっぱい なぐって こうげきする。",  // STRENGTH
  "あいての こうげきを さげて その こうげきの あたいぶん HPを かいふくする。",  // STRENGTHSAP
  "いとを ふきつけて あいての すばやさを さげる。",  // STRINGSHOT
  "ていこうして こうげきし あいての とくこうを さげる。",  // STRUGGLEBUG
  "しびれる こなを まいて あいてを まひ させる。",  // STUNSPORE
  "あいてを つかんで じめんに たたきつける。 じぶんも すこし ダメージを うける。",  // SUBMISSION
  "せんせいで こうげき。 あいてが こうげき しようと していないと しっぱいする。",  // SUCKERPUNCH
  "5ターン ひざしを つよくして ほのおわざを つよくする。",  // SUNNYDAY
  "りゅうせいの ような いきおいで あいてに とっしんする。",  // SUNSTEELSTRIKE
  "からだに でんきを まとって とびかかる。 はずれると じぶんが ダメージ。",  // SUPERCELLSLAM
  "するどい まえばで かみつき あいての HPを はんぶんに する。",  // SUPERFANG
  "すごい ちからで こうげき。 じぶんの こうげきと ぼうぎょが さがる。",  // SUPERPOWER
  "ふしぎな おんぱで こんらん させる ことが ある。",  // SUPERSONIC
  "おおきな なみで せんじょうを おしながす。",  // SURF
  "あいてを こんらん させるが あいての こうげきを ぐーんと あげてしまう。",  // SWAGGER
  "たくわえた ちからを のみこんで HPを かいふくする。",  // SWALLOW
  "かわいく キスをして あいてを こんらん させる。",  // SWEETKISS
  "あまい かおりで あいての かいひりつを さげる。 やせいの ポケモンも よびよせる。",  // SWEETSCENT
  "ほしがたの こうせんを うつ。 かならず あたる。",  // SWIFT
  "めにも とまらぬ はやさで あいてと もちものを とりかえる。",  // SWITCHEROO
  "はげしく おどって こうげきを ぐーんと あげる。",  // SWORDSDANCE
  "ふしぎな しょうげきはで じぶんと おなじ タイプの ポケモンに ダメージ。",  // SYNCHRONOISE
  "じぶんの HPを かいふく。 てんきで かいふくする りょうが かわる。",  // SYNTHESIS
  "ねばねばの みずあめで つつみ 3ターン あいての すばやさを さげる。",  // SYRUPBOMB
  "つぶの やいばを 2かい つづけて はなつ。 かならず あたる。",  // TACHYONCUTTER
  "からだごと ぶつかって こうげきする。",  // TACKLE
  "ひかりを みつめて しゅうちゅうし とくこうを ぐぐーんと あげる。",  // TAILGLOW
  "しっぽなどで 2から 5かい つづけて たたく。",  // TAILSLAP
  "しっぽを かわいく ふって あいての ぼうぎょを さげる。",  // TAILWHIP
  "おいかぜで 4ターン みかたの すばやさを 2ばいに する。",  // TAILWIND
  "はげしく とっしんする。 じぶんも ダメージを うける。",  // TAKEDOWN
  "じぶんの じょうたいいじょうを なおし とくこうと とくぼうを あげる。",  // TAKEHEART
  "タールを かけて すばやさを さげ ほのおわざに よわくする。",  // TARSHOT
  "あいてを おこらせて 3ターン こうげきわざしか だせなくする。",  // TAUNT
  "なみだめで みつめて せんいを なくさせ こうげきと とくこうを さげる。",  // TEARFULLOOK
  "おおきな レーザーで こうげき。 もっている カセットで タイプが かわる。",  // TECHNOBLAST
  "フラフラ おどって まわりの ポケモン すべてを こんらん させる。",  // TEETERDANCE
  "ステラフォルムの テラパゴスが つかうと あいて ぜんいんに ダメージ。",  // TERASTARSTORM
  "フィールドの ちからで こうげき。 フィールドで タイプと いりょくが かわる。",  // TERRAINPULSE
  "こうげきして もちものを うばう。 じぶんが もちものを もっていると うばえない。",  // THIEF
  "そらに いる ポケモンにも あたり じめんに おとす。",  // THOUSANDARROWS
  "じめんを つたう なみで あいて ぜんいんを こうげき。 あたると にげられない。",  // THOUSANDWAVES
  "2から 3ターン あばれて こうげき。 そのあと じぶんが こんらんする。",  // THRASH
  "はげしい いたみで 2ターン あいてが おとの わざを つかえなくする。",  // THROATCHOP
  "はげしい かみなりで こうげき。 まひ させる ことが ある。",  // THUNDER
  "つよい でんげきで こうげき。 まひ させる ことが ある。",  // THUNDERBOLT
  "でんきの おりに 4から 5ターン とじこめる。",  // THUNDERCAGE
  "せんせいで かみなりを おとす。 あいてが こうげき しないと しっぱいする。",  // THUNDERCLAP
  "でんきの キバで かみつく。 ひるませるか まひ させる ことが ある。",  // THUNDERFANG
  "でんきの パンチ。 まひ させる ことが ある。",  // THUNDERPUNCH
  "でんげきで こうげき。 まひ させる ことが ある。",  // THUNDERSHOCK
  "よわい でんげきを あて あいてを まひ させる。",  // THUNDERWAVE
  "くすぐって わらわせ あいての こうげきと ぼうぎょを さげる。",  // TICKLE
  "こうげきと すばやさを あげ まきびしや ステルスロック みがわりを かたづける。",  // TIDYUP
  "うたう ように ほのおを ふきだす。 じぶんの とくこうが あがる。",  // TORCHSONG
  "あいてを いらだたせ おなじ わざを 2かい つづけて だせなくする。",  // TORMENT
  "あいてを もうどくに する。 どくの ダメージは まいターン ふえる。",  // TOXIC
  "あいての ばに どくびしを まき でてきた ポケモンを どくに する。",  // TOXICSPIKES
  "どくの いとで あいての すばやさを さげ どくに する。",  // TOXICTHREAD
  "あいてと おなじ すがたと わざに へんしんする。",  // TRANSFORM
  "3ぼんの ビームで こうげき。 まひ やけど こおりに する ことが ある。",  // TRIATTACK
  "あいての すきを ついて もちものを とりかえる。",  // TRICK
  "いきの あった 3にんの ダイブで 3かい つづけて こうげきする。",  // TRIPLEDIVE
  "3かいまで つづけて こうげき。 あたるたびに いりょくが あがる。",  // TRIPLEHIT
  "3かいまで つづけて けりを はなつ。 あたるたびに いりょくが あがる。",  // TRIPLEKICK
  "みなみの くにの ちからで けり あいての こうげきを さげる。",  // TROPKICK
  "わざの のこり PPが すくないほど いりょくが あがる。",  // TRUMPCARD
  "めから ふしぎな ひかりを 2かい つづけて はなつ。",  // TWINBEAM
  "どくばりで 2かい つきさす。 どくに する ことが ある。",  // TWINEEDLE
  "はげしい たつまきで まきこむ。 ひるませる ことが ある。",  // TWISTER
  "3ターン さわいで こうげき。 そのあいだ だれも ねむれない。",  // UPROAR
  "こうげきの あと すばやく もどって てもちの ポケモンと いれかわる。",  // UTURN
  "こぶしを まわして しんくうはを うつ。 かならず せんせい できる。",  // VACUUMWAVE
  "Vの ほのおで こうげき。 じぶんの ぼうぎょ とくぼう すばやさが さがる。",  // VCREATE
  "どくの あいての こうげき とくこう すばやさを さげる。",  // VENOMDRENCH
  "どくの えきで こうげき。 あいてが どくだと ダメージが 2ばいに なる。",  // VENOSHOCK
  "ほそい ツルで あいてを たたく。",  // VINEWHIP
  "つよい ハサミで はさんで こうげきする。",  // VISEGRIP
  "さいごに こうどう するが かならず あたる。",  // VITALTHROW
  "こうげきの あと すばやく もどって てもちの ポケモンと いれかわる。",  // VOLTSWITCH
  "でんきを まとって とっしん。 じぶんも おおきな ダメージを うける。",  // VOLTTACKLE
  "ねむっている あいてに おおきな ダメージ。 そのかわり めを さます。",  // WAKEUPSLAP
  "いきおいよく つっこむ。 ひるませる ことが ある。 たきを のぼる ことも できる。",  // WATERFALL
  "いきおいよく みずを ふきつけて こうげきする。",  // WATERGUN
  "みずの はどうで こうげき。 こんらん させる ことが ある。",  // WATERPULSE
  "みずの しゅりけんで 2から 5かい こうげき。 かならず せんせい できる。",  // WATERSHURIKEN
  "みずを かぶって ばに いるあいだ ほのおわざを よわめる。",  // WATERSPORT
  "しおを ふきだして こうげき。 じぶんの HPが すくないほど いりょくが さがる。",  // WATERSPOUT
  "みずを まとって とっしん。 じぶんも おおきな ダメージを うける。",  // WAVECRASH
  "てんきで タイプと いりょくが かわる。",  // WEATHERBALL
  "はげしい うずしおに 4から 5ターン とじこめる。",  // WHIRLPOOL
  "あいてを ふきとばし こうたい させる。 やせいでは せんとうが おわる。",  // WHIRLWIND
  "あくの かたを きわめた いちげき。 かならず きゅうしょに あたる。",  // WICKEDBLOW
  "かみなりの あらしで かぜと いなずまを あびせる。 まひ させる ことが ある。",  // WILDBOLTSTORM
  "つよい でんきを まとって とっしん。 じぶんも すこし ダメージを うける。",  // WILDCHARGE
  "ぶきみな むらさきの ほのおで あいてを やけどに する。",  // WILLOWISP
  "おおきな つばさで あいてを うつ。",  // WINGATTACK
  "かたい からに こもって ぼうぎょを あげる。",  // WITHDRAW
  "がんじょうな からだで ぶつかる。 じぶんも おおきな ダメージを うける。",  // WOODHAMMER
  "ふるいたって こうげきと とくこうを あげる。",  // WORKUP
  "なやみの タネを うえつけ あいての とくせいを ふみんに する。",  // WORRYSEED
  "ツルや からだで 4から 5ターン まきつく。",  // WRAP
  "あいてを しぼりとる。 あいての HPが おおいほど いりょくが あがる。",  // WRINGOUT
  "カマや ツメを ハサミの ように こうさ させて きりさく。",  // XSCISSOR
  "おおきな あくびで つぎの ターン あいてを ねむらせる。",  // YAWN
  "でんげきを うちだす。 まひ させる ことが ある。",  // ZAPCANNON
  "あたまに ねんりきを あつめて ずつき。 ひるませる ことが ある。",  // ZENHEADBUTT
  "あいてに ぶつかって つよい でんきを ながす。 ひるませる ことが ある。",  // ZINGZAP
  "あいては もちものが つかえず トレーナーも その ポケモンに どうぐを つかえない。",  // EMBARGO
  "5ターン あいては HPを かいふくする わざ とくせい どうぐが つかえない。",  // HEALBLOCK
  "うたを きいた ポケモンは 3ターン いないに こうたい しないと ひんしに なる。",  // PERISHSONG
  "じぶんの HPを けずって みがわりを だす。",  // SUBSTITUTE
  "5ターン おそい ポケモンから さきに こうどう できる ふしぎな くうかんを つくる。",  // TRICKROOM
  "つぎの ターン さいだいHPの はんぶんを かいふくする。",  // WISH
};

static const char *const MOVE_DESC_KO[MOVE_COUNT] = {
  "쓸 수 있는 기술이 없을 때 쓴다. 자신도 데미지를 입는다.",  // STRUGGLE
  "준 데미지의 절반만큼 HP를 흡수한다.",  // ABSORB
  "엄청난 속도로 상대에게 부딪친다. 반드시 먼저 공격한다.",  // ACCELEROCK
  "강한 산을 뿌린다. 상대의 특수방어를 떨어뜨릴 때가 있다.",  // ACID
  "세포 구조를 바꿔 액체가 되어 방어를 크게 올린다.",  // ACIDARMOR
  "녹이는 액체로 공격해 상대의 특수방어를 크게 떨어뜨린다.",  // ACIDSPRAY
  "날렵하게 공격한다. 도구를 지니고 있지 않으면 큰 데미지를 준다.",  // ACROBATICS
  "몸의 급소를 눌러 능력 하나를 무작위로 크게 올린다.",  // ACUPRESSURE
  "매우 빠르게 공격해 피할 수 없다.",  // AERIALACE
  "공기의 소용돌이를 쏜다. 급소에 맞기 쉽다.",  // AEROBLAST
  "몸의 힘을 빼고 스피드를 크게 올린다.",  // AGILITY
  "날카로운 바람으로 벤다. 급소에 맞기 쉽다.",  // AIRCUTTER
  "하늘도 가르는 공기의 칼날로 공격한다. 상대를 풀죽게 할 때가 있다.",  // AIRSLASH
  "걱정을 잊어 특수방어를 크게 올린다.",  // AMNESIA
  "닻을 던져 공격한다. 상대는 붙잡혀 도망칠 수 없다.",  // ANCHORSHOT
  "원시의 힘으로 공격한다. 자신의 모든 능력이 올라갈 때가 있다.",  // ANCIENTPOWER
  "신 사과의 산성 액체로 공격해 상대의 특수방어를 떨어뜨린다.",  // APPLEACID
  "압축한 물을 칼날처럼 써서 벤다. 급소에 맞기 쉽다.",  // AQUACUTTER
  "눈에 보이지 않을 만큼 빠르게 부딪친다. 반드시 먼저 공격한다.",  // AQUAJET
  "물의 베일로 몸을 감싼다. 매 턴 HP를 조금 회복한다.",  // AQUARING
  "우아하고 유연한 춤으로 공격한다. 자신의 스피드가 올라간다.",  // AQUASTEP
  "거센 폭풍 속 성난 파도처럼 꼬리를 휘둘러 공격한다.",  // AQUATAIL
  "갑옷을 불타는 탄환처럼 쏜다. 자신의 방어와 특수방어가 떨어진다.",  // ARMORCANNON
  "팔을 뻗어 2~5회 연속으로 공격한다.",  // ARMTHRUST
  "부드러운 향기로 아군의 상태 이상을 모두 회복한다.",  // AROMATHERAPY
  "신비한 향기로 아군 포켓몬의 특수방어를 올린다.",  // AROMATICMIST
  "그 턴에 상대가 이미 데미지를 입었으면 위력이 2배가 된다.",  // ASSURANCE
  "놀라게 해서 공격한다. 상대를 풀죽게 할 때가 있다.",  // ASTONISH
  "턱으로 문다. 벌레타입 포켓몬에게 특히 효과가 좋다.",  // ATRAPAMOSCAS
  "부하들을 불러 상대를 공격하게 한다. 급소에 맞기 쉽다.",  // ATTACKORDER
  "성별이 다른 상대를 헤롱헤롱하게 만들어 공격하기 어렵게 한다.",  // ATTRACT
  "몸속에서 파동의 힘을 쏜다. 반드시 명중한다.",  // AURASPHERE
  "공격하고 자신의 스피드를 올린다. 모습에 따라 타입이 바뀐다.",  // AURAWHEEL
  "무지개색 빛을 쏜다. 상대의 공격을 떨어뜨릴 때가 있다.",  // AURORABEAM
  "몸의 일부를 떼어 가벼워져 스피드를 크게 올린다.",  // AUTOTOMIZE
  "그 턴에 자신을 공격한 상대에게 2배의 데미지를 준다.",  // AVALANCHE
  "발을 들어 뒤꿈치로 내리찍는다. 빗나가면 자신이 데미지를 입는다. 혼란시킬 때가 있다.",  // AXEKICK
  "상대를 바라보며 공격을 떨어뜨린다. 반드시 먼저 행동한다.",  // BABYDOLLEYES
  "못된 짓을 해서 상대를 공격하고 리플렉터를 친다.",  // BADDYBAD
  "공격을 막는다. 동시에 접촉한 상대를 독 상태로 만든다.",  // BANEFULBUNKER
  "독 가시를 쏘아 독 상태로 만들 때가 있다. 상대가 독이면 위력이 2배가 된다.",  // BARBBARRAGE
  "둥근 것을 상대에게 2~5회 던진다.",  // BARRAGE
  "튼튼한 벽을 만들어 방어를 크게 올린다.",  // BARRIER
  "아군과 교체하며 능력 변화를 물려준다.",  // BATONPASS
  "부리를 달군 뒤 공격한다. 그동안 접촉한 상대는 화상을 입는다.",  // BEAKBLAST
  "지닌 포켓몬 모두가 공격한다. 동료가 많을수록 공격 횟수가 늘어난다.",  // BEATUP
  "트림을 해서 데미지를 준다. 지닌 나무열매를 먹어야 쓸 수 있다.",  // BELCH
  "최대 HP의 절반을 깎아 공격을 최대로 올린다.",  // BELLYDRUM
  "2턴 동안 공격을 참은 뒤 받은 데미지의 2배로 되갚는다.",  // BIDE
  "몸이나 촉수로 4~5턴 동안 조인다.",  // BIND
  "날카로운 이빨로 문다. 상대를 풀죽게 할 때가 있다.",  // BITE
  "원한을 담아 찌른다. 준 데미지의 절반만큼 HP를 회복한다.",  // BITTERBLADE
  "차가운 원한으로 공격해 상대의 공격을 떨어뜨린다.",  // BITTERMALICE
  "급소에 맞기 쉬운 발차기. 화상을 입힐 때가 있다.",  // BLAZEKICK
  "몸이 떨리는 차가운 바람. 상대의 스피드를 떨어뜨릴 때가 있다.",  // BLEAKWINDSTORM
  "눈보라를 일으킨다. 상대를 얼릴 때가 있다.",  // BLIZZARD
  "팔을 벌려 길을 막아 상대가 도망칠 수 없게 한다.",  // BLOCK
  "아름답고 강렬한 푸른 불꽃으로 감싼다. 화상을 입힐 때가 있다.",  // BLUEFLARE
  "몸으로 부딪친다. 자신의 방어가 높을수록 데미지가 커진다.",  // BODYPRESS
  "온몸으로 덮친다. 상대를 마비시킬 때가 있다.",  // BODYSLAM
  "전기를 띤 부리로 공격한다. 상대보다 먼저 공격하면 위력이 2배가 된다.",  // BOLTBEAK
  "큰 전기를 두르고 몸통박치기한다. 상대를 마비시킬 때가 있다.",  // BOLTSTRIKE
  "뼈로 때린다. 상대를 풀죽게 할 때가 있다.",  // BONECLUB
  "뼈를 부메랑처럼 던져 2번 공격한다.",  // BONEMERANG
  "뼈를 들고 상대를 2~5회 때린다.",  // BONERUSH
  "엄청난 음파로 주위의 모든 포켓몬을 공격한다.",  // BOOMBURST
  "1턴째에 뛰어올라 2턴째에 공격한다. 마비시킬 때가 있다.",  // BOUNCE
  "물방울을 쏘아 공격하고 준 데미지의 절반을 흡수한다.",  // BOUNCYBUBBLE
  "날카로운 가지로 찔러 공격한다.",  // BRANCHPOKE
  "날개를 접고 낮게 날아 돌진한다. 자신도 반동 데미지를 입는다.",  // BRAVEBIRD
  "꼬리를 힘차게 휘둘러 공격한다. 상대의 공격을 떨어뜨린다.",  // BREAKINGSWIPE
  "강력한 공격. 빛의장막이나 리플렉터 같은 벽도 깨뜨린다.",  // BRICKBREAK
  "상대의 HP가 절반 이하이면 위력이 2배가 된다.",  // BRINE
  "몸을 휘둘러 모든 상대에게 데미지를 준다.",  // BRUTALSWING
  "거품을 뿜는다. 상대의 스피드를 떨어뜨릴 때가 있다.",  // BUBBLE
  "거품을 세차게 쏜다. 상대의 스피드를 떨어뜨릴 때가 있다.",  // BUBBLEBEAM
  "물어서 공격한다. 상대가 나무열매를 지니고 있으면 먹고 그 효과를 얻는다.",  // BUGBITE
  "날개를 떨어 음파로 공격한다. 상대의 특수방어를 떨어뜨릴 때가 있다.",  // BUGBUZZ
  "몸을 단련해 공격과 방어를 올린다.",  // BULKUP
  "땅을 흔들어 주위의 포켓몬을 공격하고 스피드를 떨어뜨린다.",  // BULLDOZE
  "탄환처럼 빠른 강한 펀치. 반드시 먼저 공격한다.",  // BULLETPUNCH
  "씨앗을 2~5회 연속으로 빠르게 쏜다.",  // BULLETSEED
  "털로 공격을 막는다. 접촉한 상대는 화상을 입을 때가 있다.",  // BURNINGBULWARK
  "몸의 불꽃을 모두 태워 공격한다. 불꽃타입이 사라진다.",  // BURNUP
  "전기를 쏘아 공격한다. 반드시 상대를 마비시킨다.",  // BUZZYBUZZ
  "마음을 가라앉히고 집중해 특수공격과 특수방어를 올린다.",  // CALMMIND
  "성별이 다른 상대를 유혹해 특수공격을 크게 떨어뜨린다.",  // CAPTIVATE
  "힘을 모아 다음 전기타입 기술의 위력을 올린다. 특수방어도 올린다.",  // CHARGE
  "전기 빔을 쏜다. 자신의 특수공격이 올라갈 때가 있다.",  // CHARGEBEAM
  "애교를 부려 상대의 공격을 크게 떨어뜨린다.",  // CHARM
  "배운 말을 음파로 만들어 공격한다. 상대를 혼란시킬 때가 있다.",  // CHATTER
  "차가운 물을 끼얹어 공격한다. 상대의 공격을 떨어뜨린다.",  // CHILLINGWATER
  "썰렁한 농담으로 눈을 내리게 하고 아군 포켓몬과 교체한다.",  // CHILLYRECEPTION
  "약점을 노려 공격한다. 상대의 능력 변화를 무시한다.",  // CHIPAWAY
  "엽록소를 모아 빔으로 쏜다. 자신도 데미지를 입는다.",  // CHLOROBLAST
  "상대를 던져 교체시킨다. 야생 포켓몬이면 배틀이 끝난다.",  // CIRCLETHROW
  "껍질로 상대를 4~5턴 동안 꽉 조인다.",  // CLAMP
  "비늘을 비벼 큰 소리로 공격한다. 자신의 방어가 떨어진다.",  // CLANGINGSCALES
  "HP를 깎아 자신의 모든 능력을 올린다.",  // CLANGOROUSSOUL
  "이상한 안개로 공격해 상대의 능력 변화를 원래대로 되돌린다.",  // CLEARSMOG
  "몸을 사리지 않고 싸운다. 자신의 방어와 특수방어가 떨어진다.",  // CLOSECOMBAT
  "똬리를 틀고 집중해 공격, 방어, 명중률을 올린다.",  // COIL
  "변신하며 땅에 부딪친다. 효과가 굉장하면 위력이 올라간다.",  // COLLISIONCOURSE
  "펀치를 2~5회 연속으로 날린다.",  // COMETPUNCH
  "마지막으로 받은 공격을 훨씬 강한 힘으로 되갚는다.",  // COMEUPPANCE
  "비밀 이야기로 집중을 흐트러뜨려 상대의 특수공격을 떨어뜨린다.",  // CONFIDE
  "수상한 빛을 쏘아 상대를 혼란시킨다.",  // CONFUSERAY
  "약한 염동력으로 공격한다. 상대를 혼란시킬 때가 있다.",  // CONFUSION
  "촉수나 덩굴로 조인다. 상대의 스피드를 떨어뜨릴 때가 있다.",  // CONSTRICT
  "직전에 쓰인 기술을 따라 한다. 아직 아무 기술도 쓰이지 않았으면 실패한다.",  // COPYCAT
  "데미지를 준다. 상대가 이미 기술을 썼으면 특성을 없앤다.",  // COREENFORCER
  "신비한 힘으로 자신의 방어와 특수방어를 올린다.",  // COSMICPOWER
  "푹신한 솜으로 몸을 감싸 방어를 매우 크게 올린다.",  // COTTONGUARD
  "솜 같은 포자를 붙여 상대의 스피드를 크게 떨어뜨린다.",  // COTTONSPORE
  "물리 공격을 받은 데미지의 2배로 되갚는다.",  // COUNTER
  "귀엽게 다가가 공격한다. 상대의 도구를 빼앗을 때가 있다.",  // COVET
  "큰 집게로 세게 내리친다. 급소에 맞기 쉽다.",  // CRABHAMMER
  "양손으로 크로스 촙을 날린다. 급소에 맞기 쉽다.",  // CROSSCHOP
  "독의 칼날로 벤다. 독 상태로 만들 때가 있다. 급소에 맞기 쉽다.",  // CROSSPOISON
  "날카로운 이빨로 깨문다. 상대의 방어를 떨어뜨릴 때가 있다.",  // CRUNCH
  "날카로운 발톱으로 찢는다. 상대의 방어를 떨어뜨릴 때가 있다.",  // CRUSHCLAW
  "상대의 HP가 많이 남아 있을수록 위력이 커진다.",  // CRUSHGRIP
  "자신이 고스트타입인지 아닌지에 따라 효과가 달라진다.",  // CURSE
  "발톱이나 낫 등으로 벤다. 가는 나무를 베어 낼 수도 있다.",  // CUT
  "회전하며 공격한다. 상대의 능력 변화를 무시한다.",  // DARKESTLARIAT
  "악의로 가득 찬 오라를 내뿜는다. 상대를 풀죽게 할 때가 있다.",  // DARKPULSE
  "상대를 어둠의 세계로 끌어들여 잠들게 한다.",  // DARKVOID
  "강렬한 빛으로 상대에게 데미지를 준다.",  // DAZZLINGGLEAM
  "부하들을 불러 살아 있는 방패를 만든다. 방어와 특수방어를 올린다.",  // DEFENDORDER
  "몸을 웅크려 약점을 숨긴다. 방어를 올린다.",  // DEFENSECURL
  "강한 바람으로 리플렉터나 빛의장막을 날린다. 상대의 회피율을 떨어뜨릴 때가 있다.",  // DEFOG
  "스키로 눈 위를 미끄러지듯 공격해 자신의 스피드를 올린다.",  // DESLIZAMIENTO
  "자신이 쓰러지면 쓰러뜨린 상대도 함께 쓰러진다.",  // DESTINYBOND
  "공격을 막는다. 연속으로 쓰면 실패하기 쉽다.",  // DETECT
  "다이아몬드의 폭풍을 일으킨다. 자신의 방어가 올라갈 때가 있다.",  // DIAMONDSTORM
  "1턴째에 땅을 파고 2턴째에 공격한다. 동굴에서 빠져나올 때도 쓴다.",  // DIG
  "치명적인 발톱으로 약점을 노린다. 독, 마비, 잠듦 중 하나로 만들 때가 있다.",  // DIRECLAW
  "상대가 마지막에 쓴 기술을 4턴 동안 쓸 수 없게 한다.",  // DISABLE
  "매혹적인 목소리로 상대에게 정신적인 데미지를 준다. 반드시 명중한다.",  // DISARMINGVOICE
  "눈부신 전기를 퍼뜨려 주위의 포켓몬을 공격한다. 마비시킬 때가 있다.",  // DISCHARGE
  "1턴째에 물속으로 잠수하고 2턴째에 떠올라 공격한다.",  // DIVE
  "리드미컬한 펀치. 상대를 혼란시킬 때가 있다.",  // DIZZYPUNCH
  "빛을 모아 2턴 뒤에 상대를 공격한다.",  // DOOMDESIRE
  "목숨을 건 돌진. 자신도 데미지를 입는다.",  // DOUBLEEDGE
  "꼬리 등으로 2번 연속 공격한다.",  // DOUBLEHIT
  "주먹으로 2번 연속 공격한다. 상대를 풀죽게 할 때가 있다.",  // DOUBLEIRONBASH
  "2번 연속으로 발차기한다.",  // DOUBLEKICK
  "몸의 전기를 모두 풀어 공격한다. 전기타입이 사라진다.",  // DOUBLESHOCK
  "2~5회 연속으로 뺨을 때린다.",  // DOUBLESLAP
  "분신을 만들어 회피율을 올린다.",  // DOUBLETEAM
  "하늘에서 큰 운석을 떨어뜨린다. 자신의 특수공격이 크게 떨어진다.",  // DRACOMETEOR
  "하늘에서 엄청난 속도로 내리꽂는다. 자신의 방어와 특수방어가 떨어진다.",  // DRAGONASCENT
  "강한 숨결을 내뿜어 공격한다. 상대를 마비시킬 때가 있다.",  // DRAGONBREATH
  "날카로운 발톱으로 할퀸다.",  // DRAGONCLAW
  "신비한 춤을 춰서 공격과 스피드를 올린다.",  // DRAGONDANCE
  "드라꼰을 날려 2번 공격한다. 상대가 둘이면 1번씩 공격한다.",  // DRAGONDARTS
  "생명력으로 공격한다. 자신의 HP가 적을수록 위력이 약해진다.",  // DRAGONENERGY
  "몸을 망치처럼 휘둘러 상대에게 덤벼들어 데미지를 준다.",  // DRAGONHAMMER
  "입을 크게 벌려 충격파를 쏘아 공격한다.",  // DRAGONPULSE
  "분노의 충격파로 공격한다. 반드시 40의 데미지를 준다.",  // DRAGONRAGE
  "상대를 위압하며 거칠게 공격한다. 상대를 풀죽게 할 때가 있다.",  // DRAGONRUSH
  "상대를 날려 교체시킨다. 야생 포켓몬이면 배틀이 끝난다.",  // DRAGONTAIL
  "입맞춤으로 상대의 HP를 흡수해 자신의 HP를 회복한다.",  // DRAININGKISS
  "에너지를 빼앗는 펀치. 준 데미지의 절반만큼 HP를 회복한다.",  // DRAINPUNCH
  "잠든 상대의 꿈을 먹는다. 준 데미지의 절반만큼 HP를 회복한다.",  // DREAMEATER
  "부리를 드릴처럼 회전시켜 강하게 찌른다.",  // DRILLPECK
  "드릴처럼 회전하며 부딪친다. 급소에 맞기 쉽다.",  // DRILLRUN
  "북을 쳐서 그루터기를 조종해 공격한다. 상대의 스피드를 떨어뜨린다.",  // DRUMBEATING
  "꼬리 등 몸의 일부로 2번 연속 공격한다.",  // DUALCHOP
  "날개로 상대를 2번 친다.",  // DUALWINGBEAT
  "코어에서 강력한 에너지 빔을 쏜다.",  // DYNAMAXCANNON
  "온 힘을 모은 펀치. 맞으면 상대를 혼란시킨다.",  // DYNAMICPUNCH
  "상대의 발밑에서 땅이 분출한다. 특수방어를 떨어뜨릴 때가 있다.",  // EARTHPOWER
  "지진을 일으켜 주위의 모든 포켓몬을 공격한다.",  // EARTHQUAKE
  "속삭이는 소리. 자신이나 다른 포켓몬이 쓸 때마다 위력이 올라간다.",  // ECHOEDVOICE
  "이상한 파동을 내뿜어 상대의 특수공격을 크게 떨어뜨린다.",  // EERIEIMPULSE
  "큰 알을 상대에게 힘껏 던진다.",  // EGGBOMB
  "5턴 동안 일렉트릭필드를 만든다. 땅에 있는 포켓몬은 잠들지 않는다.",  // ELECTRICTERRAIN
  "전기 구슬을 던진다. 상대보다 스피드가 빠를수록 데미지가 커진다.",  // ELECTROBALL
  "변신하며 덤벼들어 상대를 꿰뚫는다. 효과가 굉장하면 위력이 올라간다.",  // ELECTRODRIFT
  "1턴째에 전기를 모아 특수공격을 올리고 2턴째에 공격한다. 비가 오면 바로 공격한다.",  // ELECTROSHOT
  "전기 그물로 붙잡아 공격한다. 상대의 스피드를 떨어뜨린다.",  // ELECTROWEB
  "작은 불꽃으로 공격한다. 화상을 입힐 때가 있다.",  // EMBER
  "상대가 마지막에 쓴 기술을 3턴 동안 반복하게 한다.",  // ENCORE
  "상대의 HP를 자신의 HP와 같게 만든다.",  // ENDEAVOR
  "어떤 공격을 받아도 HP가 1 남는다. 연속으로 쓰면 실패하기 쉽다.",  // ENDURE
  "자연의 힘을 모아 쏜다. 상대의 특수방어를 떨어뜨릴 때가 있다.",  // ENERGYBALL
  "이상한 춤을 춰서 상대의 특성을 자신과 같게 만든다.",  // ENTRAINMENT
  "폭발하는 분노로 공격한다. 자신의 HP가 적을수록 위력이 약해진다.",  // ERUPTION
  "상대에게 무시무시한 한기를 느끼게 해서 얼려버린다.",  // ESCALOFRIO
  "오라를 담은 날개로 벤다. 급소에 맞기 쉽고 스피드를 올린다.",  // ESPERWING
  "원래 모습의 무한다이노의 최강 공격. 다음 턴은 움직일 수 없다.",  // ETERNABEAM
  "염동력으로 공격한다. 사이코필드에서는 위력이 올라가고 모든 상대를 공격한다.",  // EXPANDINGFORCE
  "폭발해서 주위의 모두를 공격한다. 자신은 바로 쓰러진다.",  // EXPLOSION
  "기묘한 힘으로 공격한다. 상대를 풀죽게 할 때가 있다.",  // EXTRASENSORY
  "매우 빠른 공격. 반드시 먼저 공격한다.",  // EXTREMESPEED
  "자신이 화상, 마비, 독 상태이면 위력이 2배가 된다.",  // FACADE
  "요정의 바람을 일으켜 상대에게 부딪친다.",  // FAIRYWIND
  "먼저 공격해 상대를 풀죽게 한다. 처음 나온 턴에만 쓸 수 있다.",  // FAKEOUT
  "거짓 눈물을 흘려 상대의 특수방어를 크게 떨어뜨린다.",  // FAKETEARS
  "절하는 척하며 흐트러진 머리카락으로 찌른다. 반드시 명중한다.",  // FALSESURRENDER
  "힘을 조절한 공격. 상대의 HP가 반드시 1 남는다.",  // FALSESWIPE
  "깃털로 감싸 상대의 공격을 크게 떨어뜨린다.",  // FEATHERDANCE
  "방어나 판별을 쓰는 상대도 공격하고 그 효과를 없앤다.",  // FEINT
  "상대를 방심시키고 다가가 때린다. 반드시 명중한다.",  // FEINTATTACK
  "이 기술로 상대를 쓰러뜨리면 자신의 공격이 매우 크게 올라간다.",  // FELLSTINGER
  "가끔 다른 머리들도 함께 공격해 2배의 데미지를 준다.",  // FICKLEBEAM
  "불꽃을 두르고 공격한다. 자신의 특수공격이 올라갈 때가 있다.",  // FIERYDANCE
  "목숨을 걸고 자신의 남은 HP만큼 데미지를 준다. 자신은 쓰러진다.",  // FINALGAMBIT
  "강렬한 불꽃으로 공격한다. 화상을 입힐 때가 있다.",  // FIREBLAST
  "불꽃을 두른 이빨로 문다. 풀죽게 하거나 화상을 입힐 때가 있다.",  // FIREFANG
  "달아오른 채찍으로 친다. 상대의 방어를 떨어뜨린다.",  // FIRELASH
  "불꽃 펀치. 화상을 입힐 때가 있다.",  // FIREPUNCH
  "불꽃의 소용돌이 속에 상대를 4~5턴 동안 가둔다.",  // FIRESPIN
  "위력이 높은 선제 기술. 처음 나온 턴에만 쓸 수 있다.",  // FIRSTIMPRESSION
  "아가미로 공격한다. 상대보다 먼저 공격하면 위력이 2배가 된다.",  // FISHIOUSREND
  "땅을 갈라 상대를 틈새에 떨어뜨린다. 맞으면 한 방에 쓰러뜨린다.",  // FISSURE
  "마구 날뛰어 공격한다. 자신의 HP가 적을수록 위력이 커진다.",  // FLAIL
  "불꽃을 터뜨려 공격한다. 옆의 포켓몬에게도 데미지를 준다.",  // FLAMEBURST
  "불꽃으로 공격하고 자신의 스피드를 올린다.",  // FLAMECHARGE
  "큰 불꽃으로 공격한다. 화상을 입힐 때가 있다.",  // FLAMETHROWER
  "불꽃을 두르고 돌진한다. 화상을 입힐 때가 있다.",  // FLAMEWHEEL
  "불꽃을 두르고 돌진한다. 자신도 반동 데미지를 입는다.",  // FLAREBLITZ
  "눈부신 빛으로 상대의 명중률을 떨어뜨린다.",  // FLASH
  "몸의 빛을 모아 쏜다. 상대의 특수방어를 떨어뜨릴 때가 있다.",  // FLASHCANNON
  "상대를 치켜세워 혼란시키지만 특수공격도 올려 준다.",  // FLATTER
  "강력한 빔을 쏜다. 자신의 특수공격이 크게 떨어진다.",  // FLEURCANNON
  "지닌 도구를 던진다. 도구에 따라 위력과 효과가 달라진다.",  // FLING
  "공격한 뒤 재빨리 아군 포켓몬과 교체한다.",  // FLIPTURN
  "속임수 꽃다발을 던진다. 반드시 명중하고 반드시 급소에 맞는다.",  // FLOWERTRICK
  "1턴째에 하늘로 날아올라 2턴째에 공격한다.",  // FLY
  "상대에게 몸을 날린다. 격투타입이면서 동시에 비행타입인 기술.",  // FLYINGPRESS
  "정신을 집중해 힘을 쏜다. 상대의 특수방어를 떨어뜨릴 때가 있다.",  // FOCUSBLAST
  "기를 모아 급소에 맞기 쉬워진다.",  // FOCUSENERGY
  "집중한 뒤 펀치를 날린다. 그 전에 공격을 받으면 실패한다.",  // FOCUSPUNCH
  "충격파로 공격한다. 상대를 마비시킬 때가 있다.",  // FORCEPALM
  "고스트타입에게 어떤 기술이든 맞힐 수 있고 회피율이 높은 상대도 맞힌다.",  // FORESIGHT
  "상대의 공격으로 데미지를 계산해 공격한다.",  // FOULPLAY
  "상대를 급속히 얼린다. 얼릴 때가 있다. 물타입에게 효과가 굉장하다.",  // FREEZEDRY
  "얼어붙은 안개의 결정으로 공격한다. 모두의 능력 변화를 없앤다.",  // FREEZYFROST
  "차가운 숨결을 내뿜는다. 반드시 급소에 맞는다.",  // FROSTBREATH
  "트레이너와 친밀도가 낮을수록 위력이 커진다.",  // FRUSTRATION
  "뿔이나 부리로 2~5회 연속으로 찌른다.",  // FURYATTACK
  "발톱이나 낫으로 공격한다. 연속으로 쓸수록 위력이 올라간다.",  // FURYCUTTER
  "발톱이나 낫으로 2~5회 연속으로 할퀸다.",  // FURYSWIPES
  "거대한 전기를 내리친다. 거대한 불꽃의 영향을 받으면 위력이 올라간다.",  // FUSIONBOLT
  "거대한 불꽃을 떨어뜨린다. 거대한 전기의 영향을 받으면 위력이 올라간다.",  // FUSIONFLARE
  "염동력을 모아 2턴 뒤에 상대를 공격한다.",  // FUTURESIGHT
  "위액을 뿌려 상대의 특성을 없앤다.",  // GASTROACID
  "강철 톱니바퀴 2개를 던져 2번 공격한다.",  // GEARGRIND
  "1턴째에 힘을 모아 2턴째에 특수공격, 특수방어, 스피드를 크게 올린다.",  // GEOMANCY
  "준 데미지의 절반만큼 HP를 흡수한다.",  // GIGADRAIN
  "온 힘을 다해 돌진한다. 다음 턴은 움직일 수 없다.",  // GIGAIMPACT
  "거대한 망치로 때린다. 2번 연속으로 쓸 수 없다.",  // GIGATONHAMMER
  "얼어붙는 냉기로 공격한다. 상대의 스피드를 떨어뜨린다.",  // GLACIATE
  "다음 턴에는 상대의 공격이 반드시 명중하고 2배의 데미지를 받는다.",  // GLAIVERUSH
  "무서운 눈빛으로 겁을 줘서 상대를 마비시킨다.",  // GLARE
  "염동력을 퍼부어 공격하고 빛의장막을 친다.",  // GLITZYGLOW
  "풀로 휘감아 넘어뜨린다. 상대가 무거울수록 위력이 커진다.",  // GRASSKNOT
  "기분 좋은 멜로디로 상대를 깊이 잠들게 한다.",  // GRASSWHISTLE
  "땅을 미끄러지듯 공격한다. 그래스필드에서는 먼저 공격한다.",  // GRASSYGLIDE
  "5턴 동안 그래스필드를 만든다. 땅에 있는 포켓몬은 턴이 끝날 때 HP를 회복한다.",  // GRASSYTERRAIN
  "높은 곳에서 사과를 떨어뜨려 공격한다. 상대의 방어를 떨어뜨린다.",  // GRAVAPPLE
  "귀엽게 울어서 상대의 공격을 떨어뜨린다.",  // GROWL
  "몸을 빠르게 성장시켜 공격과 특수공격을 올린다.",  // GROWTH
  "공격을 받아 쓰러지면 그 기술의 PP를 0으로 만든다.",  // GRUDGE
  "커다란 집게로 자른다. 맞으면 한 방에 쓰러뜨린다.",  // GUILLOTINE
  "더러운 쓰레기를 던진다. 독 상태로 만들 때가 있다.",  // GUNKSHOT
  "날개로 회오리바람을 일으켜 상대에게 날린다.",  // GUST
  "몸을 회전시켜 부딪친다. 자신이 느릴수록 위력이 커진다.",  // GYROBALL
  "강한 주먹을 휘둘러 내리친다. 자신의 스피드가 떨어진다.",  // HAMMERARM
  "근육에 힘을 줘서 방어를 올린다.",  // HARDEN
  "안개를 일으켜 배틀 중인 모든 포켓몬의 능력 변화를 없앤다.",  // HAZE
  "강한 박치기. 상대를 풀죽게 할 때가 있다.",  // HEADBUTT
  "머리로 힘껏 부딪친다. 자신도 반동 데미지를 조금 입는다.",  // HEADCHARGE
  "온 힘을 다해 덤벼든다. 자신의 방어와 특수방어가 떨어진다.",  // HEADLONGRUSH
  "목숨을 걸고 온 힘으로 박치기한다. 자신도 큰 반동 데미지를 입는다.",  // HEADSMASH
  "편안한 방울 소리로 아군 모두의 상태 이상을 회복한다.",  // HEALBELL
  "자신은 쓰러지지만 교체로 나온 포켓몬의 HP와 상태 이상이 회복된다.",  // HEALINGWISH
  "부하들을 불러 치료한다. 최대 HP의 절반을 회복한다.",  // HEALORDER
  "치유의 파동으로 상대의 HP를 최대 HP의 절반만큼 회복한다.",  // HEALPULSE
  "귀여운 몸짓 뒤 강하게 내리친다. 상대를 풀죽게 할 때가 있다.",  // HEARTSTAMP
  "상대보다 자신이 무거울수록 위력이 커진다.",  // HEATCRASH
  "뜨거운 바람으로 공격한다. 화상을 입힐 때가 있다.",  // HEATWAVE
  "상대보다 자신이 무거울수록 위력이 커진다.",  // HEAVYSLAM
  "상태 이상인 상대에게 큰 데미지를 준다.",  // HEX
  "쓰는 포켓몬에 따라 타입과 위력이 달라지는 기술.",  // HIDDENPOWER
  "온몸을 써서 상대를 세게 공격한다.",  // HIGHHORSEPOWER
  "높이 뛰어올라 발차기한다. 빗나가면 자신이 데미지를 입는다.",  // HIGHJUMPKICK
  "발톱을 갈아 공격과 명중률을 올린다.",  // HONECLAWS
  "뾰족한 뿔로 찔러 공격한다.",  // HORNATTACK
  "회전하는 뿔로 찌른다. 맞으면 한 방에 쓰러뜨린다.",  // HORNDRILL
  "에너지를 빼앗는 공격. 준 데미지의 절반만큼 HP를 회복한다.",  // HORNLEECH
  "힘차게 울부짖어 공격을 올린다.",  // HOWL
  "강한 회오리바람으로 감싼다. 상대를 혼란시킬 때가 있다.",  // HURRICANE
  "엄청난 양의 물을 세찬 압력으로 쏜다.",  // HYDROPUMP
  "끓는 물을 세차게 쏜다. 쾌청일 때 위력이 올라간다.",  // HYDROSTEAM
  "강렬한 광선으로 공격한다. 다음 턴은 움직일 수 없다.",  // HYPERBEAM
  "몸의 뾰족한 부분을 회전시켜 공격한다. 방어 같은 기술을 무시한다.",  // HYPERDRILL
  "날카로운 이빨로 문다. 상대를 풀죽게 할 때가 있다.",  // HYPERFANG
  "그 턴에 상대가 방어, 판별, 와이드가드, 퍼스트가드를 써도 공격한다.",  // HYPERSPACEHOLE
  "귀를 찢는 듯한 큰 소리로 데미지를 준다.",  // HYPERVOICE
  "최면을 걸어 상대를 깊이 잠들게 한다.",  // HYPNOSIS
  "5턴 동안 굴러서 공격한다. 맞힐 때마다 위력이 올라간다.",  // ICEBALL
  "얼음 빔을 쏜다. 상대를 얼릴 때가 있다.",  // ICEBEAM
  "냉기를 두른 이빨로 문다. 풀죽게 하거나 얼릴 때가 있다.",  // ICEFANG
  "무시무시한 주먹으로 내리친다. 자신의 스피드가 떨어진다.",  // ICEHAMMER
  "냉기 펀치. 상대를 얼릴 때가 있다.",  // ICEPUNCH
  "얼음 덩어리를 만들어 빠르게 던진다. 반드시 먼저 공격한다.",  // ICESHARD
  "손발에 얼음을 둘러 공격한다. 필드가 있으면 부순다.",  // ICESPINNER
  "큰 고드름을 던진다. 상대를 풀죽게 할 때가 있다.",  // ICICLECRASH
  "날카로운 고드름을 2~5회 연속으로 던진다.",  // ICICLESPEAR
  "차가운 바람으로 공격한다. 상대의 스피드를 떨어뜨린다.",  // ICYWIND
  "자신이 가진 기술을 배틀 동안 상대가 쓸 수 없게 한다.",  // IMPRISON
  "불꽃으로 옆의 상대를 공격한다. 지닌 나무열매는 타서 없어진다.",  // INCINERATE
  "강렬한 불꽃으로 감싼다. 반드시 화상을 입힌다.",  // INFERNO
  "4~5턴 동안 상대에게 달라붙어 공격한다. 그동안 상대는 도망칠 수 없다.",  // INFESTATION
  "뿌리를 내려 매 턴 HP를 회복한다. 교체할 수 없게 된다.",  // INGRAIN
  "전기를 띤 입자를 퍼뜨려 노말타입 기술을 전기타입으로 바꾼다.",  // IONDELUGE
  "몸을 철처럼 단단하게 해서 방어를 크게 올린다.",  // IRONDEFENSE
  "단단한 강철 머리로 공격한다. 상대를 풀죽게 할 때가 있다.",  // IRONHEAD
  "단단한 꼬리로 공격한다. 상대의 방어를 떨어뜨릴 때가 있다.",  // IRONTAIL
  "곤봉으로 때린다. 쓰고 있는 가면에 따라 타입이 바뀐다. 급소에 맞기 쉽다.",  // IVYCUDGEL
  "어느 한쪽이 쓰러질 때까지 상대와 자신 모두 도망칠 수 없게 된다.",  // JAWLOCK
  "주먹에 급류를 둘러 먼저 공격한다.",  // JETPUNCH
  "수많은 빛줄기를 쏜다. 지닌 플레이트에 따라 타입이 바뀐다.",  // JUDGMENT
  "뛰어올라 발차기한다. 빗나가면 자신이 데미지를 입는다.",  // JUMPKICK
  "자신과 아군의 HP를 회복하고 상태 이상도 낫게 한다.",  // JUNGLEHEALING
  "날카로운 촙으로 공격한다. 급소에 맞기 쉽다.",  // KARATECHOP
  "숟가락을 구부려 주의를 돌린다. 상대의 명중률을 떨어뜨린다.",  // KINESIS
  "공격을 막는다. 접촉한 상대의 공격을 떨어뜨린다.",  // KINGSSHIELD
  "상대가 지닌 도구를 배틀 동안 쓸 수 없게 한다.",  // KNOCKOFF
  "엎드려 절하며 방심시킨 뒤 공격한다. 반드시 명중한다.",  // KOWTOWCLEAVE
  "대지의 힘을 모아 상대에게 집중시켜 데미지를 준다.",  // LANDSWRATH
  "정신을 집중해 다음 공격이 반드시 급소에 맞게 한다.",  // LASERFOCUS
  "가지고 있는 다른 기술을 모두 쓴 뒤에만 쓸 수 있다.",  // LASTRESORT
  "쓰러진 동료의 원수를 갚는다. 쓰러진 동료가 많을수록 위력이 커진다.",  // LASTRESPECTS
  "거센 불꽃으로 주위의 포켓몬을 공격한다. 화상을 입힐 때가 있다.",  // LAVAPLUME
  "잎사귀를 날려 공격한다.",  // LEAFAGE
  "얇은 칼 같은 잎으로 벤다. 급소에 맞기 쉽다.",  // LEAFBLADE
  "날카로운 잎의 폭풍을 일으킨다. 자신의 특수공격이 크게 떨어진다.",  // LEAFSTORM
  "날카로운 잎으로 공격한다. 상대의 명중률을 떨어뜨릴 때가 있다.",  // LEAFTORNADO
  "에너지를 빨아들여 준 데미지의 절반만큼 HP를 회복한다.",  // LEECHLIFE
  "씨앗을 심어 매 턴 상대의 HP를 흡수해 자신을 회복한다.",  // LEECHSEED
  "날카로운 눈빛으로 노려봐 상대의 방어를 떨어뜨린다.",  // LEER
  "긴 혀로 핥아 공격한다. 마비시킬 때가 있다.",  // LICK
  "신비한 물을 뿌려 자신과 아군의 HP를 회복한다.",  // LIFEDEW
  "빛의 벽으로 5턴 동안 특수 공격의 데미지를 줄인다.",  // LIGHTSCREEN
  "물의 힘으로 부딪친다. 상대의 방어를 떨어뜨릴 때가 있다.",  // LIQUIDATION
  "목표를 조준해 다음 기술이 반드시 명중하게 한다.",  // LOCKON
  "무서운 얼굴로 입맞춤한다. 맞으면 상대는 잠든다.",  // LOVELYKISS
  "낮게 차서 넘어뜨린다. 상대가 무거울수록 위력이 커진다.",  // LOWKICK
  "재빠르게 발을 노려 공격한다. 상대의 스피드를 떨어뜨린다.",  // LOWSWEEP
  "하늘에 주문을 외워 상대가 급소에 맞히지 못하게 한다.",  // LUCKYCHANT
  "마음에 영향을 주는 이상한 빛으로 공격해 특수방어를 크게 떨어뜨린다.",  // LUMINACRASH
  "달에 기도해 자신 편의 HP를 회복하고 상태 이상을 낫게 한다.",  // LUNARBLESSING
  "자신은 쓰러지지만 교체로 나온 포켓몬의 HP, PP, 상태 이상이 회복된다.",  // LUNARDANCE
  "온 힘을 다해 덤벼들어 공격한다. 상대의 공격을 떨어뜨린다.",  // LUNGE
  "눈부신 빛을 내뿜는다. 상대의 특수방어를 떨어뜨릴 때가 있다.",  // LUSTERPURGE
  "번개처럼 빠른 펀치. 반드시 먼저 공격한다.",  // MACHPUNCH
  "상대를 쫓아가는 이상한 잎을 뿌린다. 반드시 명중한다.",  // MAGICALLEAF
  "불꽃의 소용돌이 속에 상대를 2~5턴 동안 가둔다.",  // MAGMASTORM
  "상대에게 달라붙는 강철 폭탄을 던진다. 반드시 명중한다.",  // MAGNETBOMB
  "전기로 만든 자기장으로 5턴 동안 공중에 뜬다.",  // MAGNETRISE
  "지진으로 주위의 모든 포켓몬을 공격한다. 위력이 매번 달라진다.",  // MAGNITUDE
  "동전을 많이 던져 공격한다. 자신의 특수공격이 떨어진다. 상금이 늘어난다.",  // MAKEITRAIN
  "사슬로 묶어 힘을 빼앗는다. 맹독 상태로 만들 때가 있다.",  // MALIGNANTCHAIN
  "차를 뿌려 공격하고 준 데미지의 절반만큼 HP를 회복한다. 화상을 입힐 때가 있다.",  // MATCHAGOTCHA
  "어두운 눈빛으로 노려봐 상대가 도망칠 수 없게 한다.",  // MEANLOOK
  "명상으로 숨은 힘을 깨워 공격을 올린다.",  // MEDITATE
  "준 데미지의 절반만큼 HP를 흡수한다.",  // MEGADRAIN
  "거대한 뿔로 거칠게 돌진한다.",  // MEGAHORN
  "엄청난 힘으로 발차기한다.",  // MEGAKICK
  "힘을 담은 펀치로 공격한다.",  // MEGAPUNCH
  "자신은 쓰러지지만 상대의 공격과 특수공격을 크게 떨어뜨린다.",  // MEMENTO
  "마지막으로 받은 공격을 훨씬 강한 힘으로 되갚는다.",  // METALBURST
  "강철 발톱으로 할퀸다. 자신의 공격이 올라갈 때가 있다.",  // METALCLAW
  "금속이 긁히는 불쾌한 소리로 상대의 특수방어를 크게 떨어뜨린다.",  // METALSOUND
  "대파를 휘둘러 공격한다. 다음 턴은 움직일 수 없다.",  // METEORASSAULT
  "1턴째에 특수공격을 올리고 2턴째에 상대를 공격한다.",  // METEORBEAM
  "유성처럼 강한 펀치. 자신의 공격이 올라갈 때가 있다.",  // METEORMASH
  "머리에 모은 빛으로 벤다. 상대가 방어해도 맞는다.",  // MIGHTYCLEAVE
  "최대 HP의 절반을 회복한다. 배틀 밖에서는 HP를 동료에게 나눠 줄 수 있다.",  // MILKDRINK
  "머리를 폭발시켜 주위의 모두를 공격한다. 자신도 데미지를 입는다.",  // MINDBLOWN
  "상대의 움직임을 읽어 다음 공격이 반드시 명중하게 한다.",  // MINDREADER
  "몸을 작게 줄여 회피율을 크게 올린다.",  // MINIMIZE
  "악타입에게 어떤 기술이든 맞힐 수 있고 회피율이 높은 상대도 맞힌다.",  // MIRACLEEYE
  "특수 공격을 받은 데미지의 2배로 되갚는다.",  // MIRRORCOAT
  "상대가 마지막에 쓴 기술을 그대로 따라 해서 반격한다.",  // MIRRORMOVE
  "윤이 나는 몸에서 빛을 쏜다. 상대의 명중률을 떨어뜨릴 때가 있다.",  // MIRRORSHOT
  "하얀 안개로 감싸 5턴 동안 아군의 능력이 떨어지지 않게 한다.",  // MIST
  "안개 같은 깃털로 감싸 공격한다. 상대의 특수공격을 떨어뜨릴 때가 있다.",  // MISTBALL
  "신비한 안개로 공격한다. 미스트필드에서는 우선도가 높아진다.",  // MISTYEXPLOSION
  "5턴 동안 땅을 안개로 덮는다. 땅에 있는 포켓몬은 상태 이상이 되지 않는다.",  // MISTYTERRAIN
  "달의 힘을 빌려 공격한다. 상대의 특수공격을 떨어뜨릴 때가 있다.",  // MOONBLAST
  "달의 신비한 빛으로 공격한다. 상대의 특성을 무시한다.",  // MOONGEISTBEAM
  "자신의 HP를 회복한다. 날씨에 따라 회복량이 달라진다.",  // MOONLIGHT
  "자신의 HP를 회복한다. 날씨에 따라 회복량이 달라진다.",  // MORNINGSUN
  "회전하며 공격해 상대를 독 상태로 만든다. 조이기나 씨뿌리기 등을 없앤다.",  // MORTALSPIN
  "빙산 같은 고드름으로 공격한다. 상대를 풀죽게 할 때가 있다.",  // MOUNTAINGALE
  "단단한 진흙 덩어리를 던진다. 상대의 명중률을 떨어뜨릴 때가 있다.",  // MUDBOMB
  "흙탕물을 쏘아 공격한다. 상대의 명중률을 떨어뜨릴 때가 있다.",  // MUDDYWATER
  "진흙을 던져 공격한다. 상대의 스피드를 떨어뜨린다.",  // MUDSHOT
  "얼굴에 진흙을 던져 공격한다. 상대의 명중률을 떨어뜨린다.",  // MUDSLAP
  "진흙을 뒤집어쓴다. 배틀에 있는 동안 전기타입 기술이 약해진다.",  // MUDSPORT
  "강력한 에너지를 둘러 부딪친다. 지닌 메모리에 따라 타입이 바뀐다.",  // MULTIATTACK
  "아주 뜨거운 불꽃을 내뿜어 상대의 특수공격을 떨어뜨린다.",  // MYSTICALFIRE
  "신비한 힘으로 공격한다. 자신의 특수공격이 올라간다.",  // MYSTICALPOWER
  "나쁜 생각을 해서 머리를 자극한다. 특수공격을 크게 올린다.",  // NASTYPLOT
  "지닌 나무열매의 힘으로 공격한다. 나무열매에 따라 타입과 위력이 바뀐다.",  // NATURALGIFT
  "자연의 분노로 상대의 HP를 절반으로 줄인다.",  // NATURESMADNESS
  "가시 돋친 팔로 때린다. 상대를 풀죽게 할 때가 있다.",  // NEEDLEARM
  "칠흑 같은 파동으로 공격한다. 상대의 명중률을 떨어뜨릴 때가 있다.",  // NIGHTDAZE
  "환영을 보여 자신의 레벨만큼 데미지를 준다.",  // NIGHTSHADE
  "빈틈을 노려 재빨리 벤다. 급소에 맞기 쉽다.",  // NIGHTSLASH
  "위협하는 함성으로 상대의 공격과 특수공격을 떨어뜨린다.",  // NOBLEROAR
  "자신의 모든 능력을 올리지만 도망치거나 교체할 수 없게 된다.",  // NORETREAT
  "전기를 띤 볼을 비벼 상대를 마비시킨다.",  // NUZZLE
  "상대의 에너지를 흡수해 준 데미지의 3/4만큼 HP를 회복한다.",  // OBLIVIONWING
  "공격을 막는다. 접촉한 상대의 방어를 크게 떨어뜨린다. 연속으로 쓰면 실패한다.",  // OBSTRUCT
  "얼굴에 먹물을 뿜는다. 상대의 명중률을 떨어뜨릴 때가 있다.",  // OCTAZOOKA
  "붙잡아 도망칠 수 없게 하고 매 턴 방어와 특수방어를 떨어뜨린다.",  // OCTOLOCK
  "고스트타입에게 어떤 기술이든 맞힐 수 있고 회피율이 높은 상대도 맞힌다.",  // ODORSLEUTH
  "섬뜩한 바람으로 공격한다. 자신의 모든 능력이 올라갈 때가 있다.",  // OMINOUSWIND
  "입속에 싸리용이 있으면 그 모습에 따라 능력 하나를 올린다.",  // ORDERUP
  "푸른빛의 수많은 광선으로 공격한다.",  // ORIGINPULSE
  "2~3턴 동안 날뛰며 공격한다. 그 뒤 자신은 혼란에 빠진다.",  // OUTRAGE
  "몸을 튕겨 엄청난 진동을 일으켜 공격한다.",  // OVERDRIVE
  "온 힘으로 공격한다. 자신의 특수공격이 크게 떨어진다.",  // OVERHEAT
  "자신과 상대의 HP를 더해 똑같이 나눈다.",  // PAINSPLIT
  "주위의 포켓몬을 공격하고 준 데미지의 절반만큼 HP를 회복한다.",  // PARABOLICCHARGE
  "상대의 공격과 특수공격을 떨어뜨리고 아군 포켓몬과 교체한다.",  // PARTINGSHOT
  "상대가 먼저 기술을 쓰면 위력이 2배가 된다.",  // PAYBACK
  "동전을 던져 공격한다. 배틀이 끝나면 돈을 얻는다.",  // PAYDAY
  "날카로운 뿔이나 부리로 찌른다.",  // PECK
  "거센 꽃잎 바람으로 주위의 모든 포켓몬을 공격한다.",  // PETALBLIZZARD
  "2~3턴 동안 꽃잎을 흩뿌리며 공격한다. 그 뒤 자신은 혼란에 빠진다.",  // PETALDANCE
  "1턴째에 사라지고 2턴째에 공격한다. 상대가 방어해도 맞는다.",  // PHANTOMFORCE
  "공격과 특수공격 중 높은 쪽으로 데미지를 준다.",  // PHOTONGEYSER
  "날카로운 가시를 2~5회 연속으로 쏜다.",  // PINMISSILE
  "주먹으로 공격한다. 노말타입 기술을 전기타입으로 바꾼다.",  // PLASMAFISTS
  "친구가 되어 싸울 마음을 없앤다. 상대의 공격을 떨어뜨린다.",  // PLAYNICE
  "장난치며 공격한다. 상대의 공격을 떨어뜨릴 때가 있다.",  // PLAYROUGH
  "부리로 쫀다. 상대가 나무열매를 지니고 있으면 먹고 그 효과를 얻는다.",  // PLUCK
  "독이 있는 이빨로 문다. 맹독 상태로 만들 때가 있다.",  // POISONFANG
  "독가스를 얼굴에 뿌려 상대를 독 상태로 만든다.",  // POISONGAS
  "독이 있는 촉수나 팔로 찌른다. 독 상태로 만들 때가 있다.",  // POISONJAB
  "독가루를 뿌려 상대를 독 상태로 만든다.",  // POISONPOWDER
  "독침으로 찌른다. 독 상태로 만들 때가 있다.",  // POISONSTING
  "꼬리로 공격한다. 독 상태로 만들 때가 있고 급소에 맞기 쉽다.",  // POISONTAIL
  "폭발하는 경단으로 공격한다. 아군에게 맞으면 HP를 회복시킨다.",  // POLLENPUFF
  "상대가 지닌 도구를 조종해 공격한다. 도구가 없으면 실패한다.",  // POLTERGEIST
  "동료들과 함께 1~10회 연속으로 공격한다.",  // POPULATIONBOMB
  "상대에게 덤벼들어 공격한다. 상대의 스피드를 떨어뜨린다.",  // POUNCE
  "꼬리나 다리 등으로 때린다.",  // POUND
  "차가운 가루눈을 날린다. 상대를 얼릴 때가 있다.",  // POWDERSNOW
  "수많은 보석처럼 반짝이는 빛으로 공격한다.",  // POWERGEM
  "자신의 능력이 많이 올라가 있을수록 위력이 커진다.",  // POWERTRIP
  "때릴 때마다 주먹이 단단해진다. 맞으면 자신의 공격이 올라간다.",  // POWERUPPUNCH
  "덩굴이나 촉수를 휘둘러 세차게 때린다.",  // POWERWHIP
  "대지의 힘을 날카로운 칼날로 바꿔 공격한다.",  // PRECIPICEBLADES
  "함정이 든 선물을 준다. 가끔 HP를 회복시킨다.",  // PRESENT
  "프리즘으로 강력한 광선을 쏜다. 다음 턴은 움직일 수 없다.",  // PRISMATICLASER
  "공격을 막는다. 연속으로 쓰면 실패하기 쉽다.",  // PROTECT
  "이상한 광선으로 공격한다. 상대를 혼란시킬 때가 있다.",  // PSYBEAM
  "실체 없는 칼날로 벤다. 일렉트릭필드에서는 위력이 올라간다.",  // PSYBLADE
  "강한 염동력으로 공격한다. 상대의 특수방어를 떨어뜨릴 때가 있다.",  // PSYCHIC
  "염동력으로 문다. 빛의장막이나 리플렉터 같은 벽도 깨뜨린다.",  // PSYCHICFANGS
  "5턴 동안 에스퍼타입 기술이 강해지고 선제 기술은 실패한다.",  // PSYCHICTERRAIN
  "온 힘으로 공격한다. 자신의 특수공격이 크게 떨어진다.",  // PSYCHOBOOST
  "염동력으로 만든 칼날로 벤다. 급소에 맞기 쉽다.",  // PSYCHOCUT
  "자기 암시를 걸어 상대의 능력 변화를 그대로 따라 한다.",  // PSYCHUP
  "염동력을 두르고 부딪친다. 자신의 방어도 올라간다.",  // PSYSHIELDBASH
  "이상한 염동력의 파동으로 공격해 큰 물리 데미지를 준다.",  // PSYSHOCK
  "이상한 염동력의 파동으로 공격해 물리 데미지를 준다.",  // PSYSTRIKE
  "세기가 매번 달라지는 이상한 염동력의 파동으로 공격한다.",  // PSYWAVE
  "상대의 능력이 많이 올라가 있을수록 위력이 커진다.",  // PUNISHMENT
  "상대의 상태 이상을 낫게 하고 그 대신 자신의 HP를 회복한다.",  // PURIFY
  "교체하려는 상대에게 2배의 데미지를 준다.",  // PURSUIT
  "작은 돌에 불을 붙여 불꽃 구슬로 공격한다. 화상을 입힐 때가 있다.",  // PYROBALL
  "눈에 보이지 않을 만큼 빠르게 부딪친다. 반드시 먼저 공격한다.",  // QUICKATTACK
  "선제 기술로부터 자신과 아군을 지킨다. 연속으로 쓰면 실패하기 쉽다.",  // QUICKGUARD
  "신비한 춤을 춰서 특수공격, 특수방어, 스피드를 올린다.",  // QUIVERDANCE
  "이 기술을 쓰는 동안 공격을 받을 때마다 공격이 올라간다.",  // RAGE
  "분노를 힘으로 바꾼다. 공격을 많이 받았을수록 위력이 커진다.",  // RAGEFIST
  "모습에 따라 타입이 바뀌는 기술. 벽을 깨뜨린다.",  // RAGINGBULL
  "2~3턴 동안 거센 불꽃을 뿜으며 날뛴다. 그 뒤 자신은 혼란에 빠진다.",  // RAGINGFURY
  "5턴 동안 비를 내려 물타입 기술의 위력을 올린다.",  // RAINDANCE
  "회전하며 공격해 스피드를 올린다. 조이기, 씨뿌리기, 압정뿌리기 등을 없앤다.",  // RAPIDSPIN
  "날카로운 잎으로 벤다. 급소에 맞기 쉽다.",  // RAZORLEAF
  "날카로운 조개껍질로 벤다. 상대의 방어를 떨어뜨릴 때가 있다.",  // RAZORSHELL
  "바람의 칼날로 벤다. 급소에 맞기 쉽다.",  // RAZORWIND
  "최대 HP의 절반까지 회복한다.",  // RECOVER
  "배틀에서 이미 쓴 일회용 도구를 다시 쓸 수 있게 한다.",  // RECYCLE
  "빛의 벽으로 5턴 동안 물리 공격의 데미지를 줄인다.",  // REFLECT
  "몸을 쉬게 해서 마비, 독, 화상을 낫게 한다.",  // REFRESH
  "옛 노래로 주위의 상대를 감동시켜 공격한다. 잠들게 할 때가 있다.",  // RELICSONG
  "HP와 상태 이상을 회복한다. 다음 2턴 동안 잠든다.",  // REST
  "앞 턴에 동료가 쓰러졌으면 위력이 올라간다.",  // RETALIATE
  "트레이너와 친밀도가 높을수록 위력이 커진다.",  // RETURN
  "힘찬 춤. 자신의 첫 번째 타입이 되고 그것이 없으면 두 번째 타입이 된다.",  // REVELATIONDANCE
  "그 턴에 상대에게 공격을 받았으면 2배의 데미지를 준다.",  // REVENGE
  "필사의 공격. 자신의 HP가 적을수록 위력이 커진다.",  // REVERSAL
  "상대가 일렉트릭필드의 영향을 받고 있으면 위력이 2배가 된다.",  // RISINGVOLTAGE
  "상대를 겁줘서 교체시킨다. 야생 포켓몬이면 배틀이 끝난다.",  // ROAR
  "시간도 일그러뜨리는 힘으로 공격한다. 다음 턴은 움직일 수 없다.",  // ROAROFTIME
  "단단한 바위를 2~5회 연속으로 던진다.",  // ROCKBLAST
  "엄청난 기세로 덤벼든다. 상대를 혼란시킬 때가 있다.",  // ROCKCLIMB
  "몸을 닦아 공기 저항을 줄여 스피드를 크게 올린다.",  // ROCKPOLISH
  "큰 바위를 던진다. 상대를 풀죽게 할 때가 있다.",  // ROCKSLIDE
  "바위도 부수는 주먹으로 때린다. 상대의 방어를 떨어뜨릴 때가 있다.",  // ROCKSMASH
  "작은 돌을 들어 던진다.",  // ROCKTHROW
  "바위를 던져 가로막는다. 상대의 스피드를 떨어뜨린다.",  // ROCKTOMB
  "거대한 바위를 날린다. 다음 턴은 움직일 수 없다.",  // ROCKWRECKER
  "빠르게 몸을 돌려 발차기한다. 상대를 풀죽게 할 때가 있다.",  // ROLLINGKICK
  "몸을 말아 바퀴처럼 배틀필드를 굴러다니며 공격한다.",  // ROLLOUT
  "땅에 내려앉아 쉰다. 최대 HP의 절반까지 회복한다.",  // ROOST
  "노래로 공격한다. 쓰는 포켓몬이 많을수록 위력이 올라간다.",  // ROUND
  "엄청난 재앙을 일으켜 상대의 HP를 절반으로 줄인다.",  // RUINATION
  "강렬한 신비의 불꽃으로 공격한다. 화상을 입힐 때가 있다.",  // SACREDFIRE
  "검으로 공격한다. 상대의 능력 변화를 무시한다.",  // SACREDSWORD
  "5턴 동안 상태 이상이 되지 않는 보호막을 친다.",  // SAFEGUARD
  "상대는 매 턴 HP가 줄어든다. 강철타입과 물타입은 더 많이 줄어든다.",  // SALTCURE
  "얼굴에 모래를 뿌려 상대의 명중률을 떨어뜨린다.",  // SANDATTACK
  "뜨거운 모래로 감싸 공격한다. 화상을 입힐 때가 있다.",  // SANDSEARSTORM
  "5턴 동안 모래바람이 분다. 바위, 땅, 강철타입 외의 포켓몬은 데미지를 입는다.",  // SANDSTORM
  "거센 모래 소용돌이 속에 상대를 4~5턴 동안 가둔다.",  // SANDTOMB
  "씨앗을 흩뿌려 공격하고 상대에게 씨뿌리기를 심는다.",  // SAPPYSEED
  "뜨거운 물을 쏘아 공격한다. 화상을 입힐 때가 있다.",  // SCALD
  "비늘을 2~5회 쏜다. 자신의 스피드가 올라가고 방어가 떨어진다.",  // SCALESHOT
  "무서운 얼굴로 겁을 줘서 상대의 스피드를 크게 떨어뜨린다.",  // SCARYFACE
  "뜨겁게 달군 모래를 던져 공격한다. 화상을 입힐 때가 있다.",  // SCORCHINGSANDS
  "날카로운 발톱으로 할퀸다.",  // SCRATCH
  "귀를 찢는 소리를 내서 상대의 방어를 크게 떨어뜨린다.",  // SCREECH
  "지옥 같은 불꽃으로 주위의 포켓몬을 공격한다. 화상을 입힐 때가 있다.",  // SEARINGSHOT
  "비밀의 힘으로 공격한다. 장소에 따라 추가 효과가 달라진다.",  // SECRETPOWER
  "긴 칼로 꿰뚫어 신비한 힘으로 데미지를 준다.",  // SECRETSWORD
  "터지는 씨앗을 위에서 퍼붓는다.",  // SEEDBOMB
  "몸속에서 충격파를 일으킨다. 상대의 특수방어를 떨어뜨릴 때가 있다.",  // SEEDFLARE
  "중력을 이용해 내던진다. 자신의 레벨만큼 데미지를 준다.",  // SEISMICTOSS
  "폭발해서 주위의 모두를 공격한다. 자신은 바로 쓰러진다.",  // SELFDESTRUCT
  "검은 그림자 덩어리를 던진다. 상대의 특수방어를 떨어뜨릴 때가 있다.",  // SHADOWBALL
  "영혼이 깃든 뼈로 때린다. 상대의 방어를 떨어뜨릴 때가 있다.",  // SHADOWBONE
  "그림자로 만든 날카로운 발톱으로 벤다. 급소에 맞기 쉽다.",  // SHADOWCLAW
  "1턴째에 사라지고 2턴째에 공격한다. 상대가 방어해도 맞는다.",  // SHADOWFORCE
  "그림자 속에서 날리는 펀치. 반드시 명중한다.",  // SHADOWPUNCH
  "그림자를 뻗어 뒤에서 공격한다. 반드시 먼저 공격한다.",  // SHADOWSNEAK
  "몸의 모서리를 날카롭게 해서 공격을 올린다.",  // SHARPEN
  "절대영도의 냉기로 공격한다. 맞으면 한 방에 쓰러뜨린다.",  // SHEERCOLD
  "껍질을 깨서 방어와 특수방어가 떨어지지만 공격, 특수공격, 스피드를 크게 올린다.",  // SHELLSMASH
  "물리 공격을 받으면 껍질의 함정이 터져 상대에게 데미지를 준다.",  // SHELLTRAP
  "매우 빠른 전격. 반드시 명중한다.",  // SHOCKWAVE
  "최대 HP의 절반을 회복한다. 모래바람일 때는 더 많이 회복한다.",  // SHOREUP
  "수상한 빛으로 공격한다. 상대를 혼란시킬 때가 있다.",  // SIGNALBEAM
  "실로 만든 함정으로 자신을 지킨다. 접촉한 상대의 스피드를 떨어뜨린다.",  // SILKTRAP
  "비늘 가루를 실은 바람으로 공격한다. 자신의 모든 능력이 올라갈 때가 있다.",  // SILVERWIND
  "부드러운 자장가로 상대를 깊이 잠들게 한다.",  // SING
  "불꽃을 두르고 돌진한다. 반드시 화상을 입힌다.",  // SIZZLYSLIDE
  "상대가 마지막에 쓴 기술을 영원히 배운다. 한 번 쓰면 이 기술은 사라진다.",  // SKETCH
  "몰래 뒤로 돌아가 공격한다. 상대의 특수공격을 떨어뜨린다.",  // SKITTERSMACK
  "1턴째에 머리를 넣어 방어를 올리고 2턴째에 박치기한다.",  // SKULLBASH
  "2턴에 걸쳐 공격한다. 급소에 맞기 쉽고 풀죽게 할 때가 있다.",  // SKYATTACK
  "힘차게 위로 올려 치는 어퍼컷.",  // SKYUPPERCUT
  "게으름을 피우며 최대 HP의 절반까지 회복한다.",  // SLACKOFF
  "꼬리나 덩굴 등으로 내리친다.",  // SLAM
  "칼날이나 집게로 벤다. 급소에 맞기 쉽다.",  // SLASH
  "수면가루를 뿌려 상대를 잠들게 한다.",  // SLEEPPOWDER
  "오물을 던진다. 독 상태로 만들 때가 있다.",  // SLUDGE
  "오물을 던진다. 독 상태로 만들 때가 있다.",  // SLUDGEBOMB
  "독의 파도로 주위의 포켓몬을 공격한다. 독 상태로 만들 때가 있다.",  // SLUDGEWAVE
  "돌 등을 던져 공격한다. 날고 있는 상대는 땅으로 떨어진다.",  // SMACKDOWN
  "날카로운 뿔로 찌른다. 반드시 명중한다.",  // SMARTSTRIKE
  "마비된 상대에게 2배의 데미지를 주지만 마비가 낫는다.",  // SMELLINGSALTS
  "더러운 가스로 공격한다. 독 상태로 만들 때가 있다.",  // SMOG
  "연기나 먹물로 상대의 명중률을 떨어뜨린다.",  // SMOKESCREEN
  "덫으로 상대를 4~5턴 동안 붙잡는다.",  // SNAPTRAP
  "참을 수 없는 소리로 공격한다. 상대의 특수공격을 떨어뜨린다.",  // SNARL
  "공격을 끌어들이는 특성이나 기술을 무시하고 상대를 공격한다.",  // SNIPESHOT
  "잠든 상태에서만 쓸 수 있는 큰 코골이. 상대를 풀죽게 할 때가 있다.",  // SNORE
  "5턴 동안 눈을 내리게 한다. 얼음타입 포켓몬의 방어가 올라간다.",  // SNOWSCAPE
  "많은 물을 끼얹어 상대를 물타입으로 만든다.",  // SOAK
  "최대 HP의 절반을 회복한다. 배틀 밖에서는 HP를 동료에게 나눠 준다.",  // SOFTBOILED
  "1턴째에 빛을 모으고 2턴째에 공격한다.",  // SOLARBEAM
  "1턴째에 빛을 모으고 2턴째에 빛의 칼날로 공격한다.",  // SOLARBLADE
  "충격파로 공격한다. 반드시 20의 데미지를 준다.",  // SONICBOOM
  "상대를 주위의 공간째로 찢는다. 급소에 맞기 쉽다.",  // SPACIALREND
  "전기를 두르고 부딪친다. 상대를 마비시킬 때가 있다.",  // SPARK
  "노래하며 물방울을 퍼뜨린다. 맞은 포켓몬의 화상이 낫는다.",  // SPARKLINGARIA
  "회오리로 감싸 공격한다. 아군 모두의 상태 이상을 낫게 한다.",  // SPARKLYSWIRL
  "상대의 그림자에 숨어 능력 상승을 빼앗은 뒤 공격한다.",  // SPECTRALTHIEF
  "끈적한 실로 휘감아 상대가 도망칠 수 없게 한다.",  // SPIDERWEB
  "날카로운 가시를 2~5회 연속으로 쏜다.",  // SPIKECANNON
  "상대 쪽에 압정을 뿌린다. 교체로 나온 상대 포켓몬은 데미지를 입는다.",  // SPIKES
  "공격을 막는다. 접촉한 상대에게 데미지를 준다.",  // SPIKYSHIELD
  "격렬하게 회전한다. 자신의 스피드가 크게 떨어진다.",  // SPINOUT
  "기세 좋게 공격해 기를 꺾는다. 상대의 특수공격을 떨어뜨린다.",  // SPIRITBREAK
  "공격하며 그림자를 땅에 꿰매 상대가 도망칠 수 없게 한다.",  // SPIRITSHACKLE
  "원한을 품고 상대가 마지막에 쓴 기술의 PP를 4 줄인다.",  // SPITE
  "비축하기로 모은 힘을 한 번에 풀어 공격한다.",  // SPITUP
  "아무 효과도 없다. 그냥 튀어 오를 뿐이다.",  // SPLASH
  "포자를 뿌려 상대를 잠들게 한다.",  // SPORE
  "사랑과 증오의 폭풍으로 감싸 공격한다. 상대의 공격을 떨어뜨릴 때가 있다.",  // SPRINGTIDESTORM
  "공중에 떠 있는 바위 함정. 교체로 나온 상대 포켓몬에게 데미지를 준다.",  // STEALTHROCK
  "아주 뜨거운 증기로 감싼다. 화상을 입힐 때가 있다.",  // STEAMERUPTION
  "몸을 공처럼 말아 굴러서 깔아뭉갠다. 상대를 풀죽게 할 때가 있다.",  // STEAMROLLER
  "몸에 모은 강철을 빔으로 쏜다. 자신도 데미지를 입는다.",  // STEELBEAM
  "공격하며 필드를 부순다. 필드가 없으면 실패한다.",  // STEELROLLER
  "단단한 날개로 친다. 자신의 방어가 올라갈 때가 있다.",  // STEELWING
  "상대 쪽에 끈적한 그물을 친다. 교체로 나온 포켓몬의 스피드를 떨어뜨린다.",  // STICKYWEB
  "힘을 모아 방어와 특수방어를 올린다. 3번까지 쓸 수 있다.",  // STOCKPILE
  "세차게 짓밟는다. 상대를 풀죽게 할 때가 있다.",  // STOMP
  "분함을 담아 공격한다. 앞서 쓴 기술이 실패했으면 위력이 2배가 된다.",  // STOMPINGTANTRUM
  "돌도끼로 공격한다. 흩어진 돌 조각이 상대 주위에 떠다닌다.",  // STONEAXE
  "날카로운 돌을 꽂는다. 급소에 맞기 쉽다.",  // STONEEDGE
  "자신의 능력이 많이 올라가 있을수록 위력이 커진다.",  // STOREDPOWER
  "강렬한 일격을 날린다. 반드시 급소에 맞는다.",  // STORMTHROW
  "증기를 내뿜어 공격한다. 상대를 혼란시킬 때가 있다.",  // STRANGESTEAM
  "강한 펀치로 공격한다.",  // STRENGTH
  "상대의 공격을 떨어뜨리고 상대의 공격 수치만큼 HP를 회복한다.",  // STRENGTHSAP
  "실을 뿜어 상대의 스피드를 떨어뜨린다.",  // STRINGSHOT
  "저항하며 공격한다. 상대의 특수공격을 떨어뜨린다.",  // STRUGGLEBUG
  "저리게 하는 가루를 뿌려 상대를 마비시킨다.",  // STUNSPORE
  "상대를 붙잡고 땅에 내던진다. 자신도 데미지를 입는다.",  // SUBMISSION
  "먼저 공격할 수 있다. 상대가 공격하려는 게 아니면 실패한다.",  // SUCKERPUNCH
  "5턴 동안 햇살을 강하게 해서 불꽃타입 기술의 위력을 올린다.",  // SUNNYDAY
  "유성 같은 기세로 돌진해 공격한다.",  // SUNSTEELSTRIKE
  "몸에 전기를 둘러 덮친다. 빗나가면 자신이 데미지를 입는다.",  // SUPERCELLSLAM
  "날카로운 이빨로 물어 상대의 HP를 절반으로 줄인다.",  // SUPERFANG
  "강력한 공격. 자신의 공격과 방어가 떨어진다.",  // SUPERPOWER
  "이상한 음파를 내서 상대를 혼란시킬 때가 있다.",  // SUPERSONIC
  "거대한 파도로 배틀필드를 휩쓴다.",  // SURF
  "상대를 혼란시키지만 상대의 공격도 크게 올려 준다.",  // SWAGGER
  "비축하기로 모은 힘을 삼켜 HP를 회복한다.",  // SWALLOW
  "천사처럼 귀엽게 입맞춤해서 상대를 혼란시킨다.",  // SWEETKISS
  "달콤한 향기로 상대의 회피율을 떨어뜨린다. 야생 포켓몬도 꾀어낸다.",  // SWEETSCENT
  "별 모양의 빛을 쏜다. 반드시 명중한다.",  // SWIFT
  "눈에 보이지 않을 만큼 빠르게 상대와 지닌 도구를 바꾼다.",  // SWITCHEROO
  "격렬한 춤을 춰서 공격을 크게 올린다.",  // SWORDSDANCE
  "이상한 충격파로 자신과 같은 타입의 주위 포켓몬에게 데미지를 준다.",  // SYNCHRONOISE
  "자신의 HP를 회복한다. 날씨에 따라 회복량이 달라진다.",  // SYNTHESIS
  "끈적한 꿀을 뒤집어씌워 3턴 동안 상대의 스피드를 떨어뜨린다.",  // SYRUPBOMB
  "입자로 만든 칼날을 날려 2번 연속 공격한다. 반드시 명중한다.",  // TACHYONCUTTER
  "몸통으로 부딪쳐 공격한다.",  // TACKLE
  "빛을 바라보며 집중해 특수공격을 매우 크게 올린다.",  // TAILGLOW
  "꼬리 등으로 2~5회 연속으로 때린다.",  // TAILSLAP
  "꼬리를 귀엽게 흔들어 상대의 방어를 떨어뜨린다.",  // TAILWHIP
  "거센 순풍을 일으켜 4턴 동안 아군 포켓몬의 스피드를 2배로 한다.",  // TAILWIND
  "거칠게 돌진한다. 자신도 반동 데미지를 입는다.",  // TAKEDOWN
  "자신의 상태 이상을 낫게 하고 특수공격과 특수방어를 올린다.",  // TAKEHEART
  "끈적한 타르를 뿌려 스피드를 떨어뜨리고 불꽃타입 기술에 약하게 만든다.",  // TARSHOT
  "상대를 도발해 3턴 동안 공격 기술만 쓰게 한다.",  // TAUNT
  "눈물 어린 눈으로 투지를 꺾어 상대의 공격과 특수공격을 떨어뜨린다.",  // TEARFULLOOK
  "큰 레이저를 쏜다. 지닌 카세트에 따라 타입이 바뀐다.",  // TECHNOBLAST
  "흐느적거리는 춤으로 주위의 포켓몬을 모두 혼란시킨다.",  // TEETERDANCE
  "스텔라폼의 테라파고스가 쓰면 모든 상대에게 데미지를 준다.",  // TERASTARSTORM
  "필드의 힘으로 공격한다. 필드에 따라 타입과 위력이 바뀐다.",  // TERRAINPULSE
  "공격하며 상대의 도구를 빼앗는다. 자신이 도구를 지니고 있으면 빼앗지 못한다.",  // THIEF
  "공중에 떠 있는 포켓몬에게도 맞고 땅으로 떨어뜨린다.",  // THOUSANDARROWS
  "땅을 타고 퍼지는 파동으로 모든 상대를 공격한다. 맞은 상대는 도망칠 수 없다.",  // THOUSANDWAVES
  "2~3턴 동안 날뛰며 공격한다. 그 뒤 자신은 혼란에 빠진다.",  // THRASH
  "목을 쳐서 고통을 줘 2턴 동안 소리 기술을 쓸 수 없게 한다.",  // THROATCHOP
  "강력한 번개로 공격한다. 상대를 마비시킬 때가 있다.",  // THUNDER
  "강한 전기로 공격한다. 상대를 마비시킬 때가 있다.",  // THUNDERBOLT
  "전기 우리에 상대를 4~5턴 동안 가둔다.",  // THUNDERCAGE
  "먼저 번개를 떨어뜨린다. 상대가 공격하려는 게 아니면 실패한다.",  // THUNDERCLAP
  "전기를 두른 이빨로 문다. 풀죽게 하거나 마비시킬 때가 있다.",  // THUNDERFANG
  "전기 펀치. 상대를 마비시킬 때가 있다.",  // THUNDERPUNCH
  "전기로 공격한다. 상대를 마비시킬 때가 있다.",  // THUNDERSHOCK
  "약한 전기를 흘려 맞으면 상대를 마비시킨다.",  // THUNDERWAVE
  "간지럽혀 웃게 만들어 상대의 공격과 방어를 떨어뜨린다.",  // TICKLE
  "공격과 스피드를 올린다. 압정뿌리기, 스텔스록, 대타출동 등을 없앤다.",  // TIDYUP
  "노래하듯 뜨거운 불꽃을 내뿜는다. 자신의 특수공격이 올라간다.",  // TORCHSONG
  "상대를 괴롭혀 같은 기술을 2번 연속으로 쓸 수 없게 한다.",  // TORMENT
  "상대를 맹독 상태로 만든다. 독 데미지가 턴마다 커진다.",  // TOXIC
  "상대 쪽에 독압정을 뿌린다. 교체로 나온 포켓몬은 독에 걸린다.",  // TOXICSPIKES
  "독이 있는 실로 공격해 스피드를 떨어뜨리고 독 상태로 만든다.",  // TOXICTHREAD
  "상대와 똑같은 모습으로 변신해 같은 기술을 쓴다.",  // TRANSFORM
  "3가지 광선으로 공격한다. 마비, 화상, 얼음 중 하나로 만들 때가 있다.",  // TRIATTACK
  "상대의 빈틈을 노려 서로의 도구를 바꾼다.",  // TRICK
  "완벽한 호흡으로 3번 연속 뛰어들어 공격한다.",  // TRIPLEDIVE
  "3번까지 연속으로 때린다. 맞힐 때마다 위력이 올라간다.",  // TRIPLEHIT
  "3번까지 연속으로 발차기한다. 맞힐 때마다 위력이 올라간다.",  // TRIPLEKICK
  "남국의 힘을 담아 발차기한다. 상대의 공격을 떨어뜨린다.",  // TROPKICK
  "남은 PP가 적을수록 위력이 커진다.",  // TRUMPCARD
  "눈에서 신비한 광선 2줄기를 쏘아 2번 연속 공격한다.",  // TWINBEAM
  "독침으로 2번 찌른다. 독 상태로 만들 때가 있다.",  // TWINEEDLE
  "거센 회오리를 일으켜 공격한다. 상대를 풀죽게 할 때가 있다.",  // TWISTER
  "3턴 동안 소란을 피우며 공격한다. 그동안 모두 잠들 수 없다.",  // UPROAR
  "공격한 뒤 재빨리 돌아와 아군 포켓몬과 교체한다.",  // UTURN
  "주먹을 돌려 진공의 파동을 날린다. 반드시 먼저 공격한다.",  // VACUUMWAVE
  "V자 불꽃으로 공격한다. 자신의 방어, 특수방어, 스피드가 떨어진다.",  // VCREATE
  "독 상태인 상대의 공격, 특수공격, 스피드를 떨어뜨린다.",  // VENOMDRENCH
  "독액을 끼얹어 공격한다. 상대가 독 상태이면 위력이 2배가 된다.",  // VENOSHOCK
  "채찍 같은 가는 덩굴로 때린다.",  // VINEWHIP
  "강한 집게로 꽉 집어 공격한다.",  // VISEGRIP
  "마지막에 공격하지만 반드시 명중한다.",  // VITALTHROW
  "공격한 뒤 재빨리 돌아와 아군 포켓몬과 교체한다.",  // VOLTSWITCH
  "전기를 두르고 돌진한다. 자신도 큰 반동 데미지를 입는다.",  // VOLTTACKLE
  "잠든 상대에게 큰 데미지를 주지만 상대는 잠에서 깬다.",  // WAKEUPSLAP
  "기세 좋게 돌진한다. 풀죽게 할 때가 있다. 폭포를 오를 수도 있다.",  // WATERFALL
  "물을 세차게 쏘아 공격한다.",  // WATERGUN
  "강한 물줄기로 공격한다. 상대를 혼란시킬 때가 있다.",  // WATERPULSE
  "물의 수리검으로 2~5회 공격한다. 반드시 먼저 공격한다.",  // WATERSHURIKEN
  "물을 뒤집어쓴다. 배틀에 있는 동안 불꽃타입 기술이 약해진다.",  // WATERSPORT
  "물을 뿜어 공격한다. 자신의 HP가 적을수록 위력이 약해진다.",  // WATERSPOUT
  "물을 두르고 돌진한다. 자신도 큰 반동 데미지를 입는다.",  // WAVECRASH
  "날씨에 따라 타입과 위력이 바뀌는 공격.",  // WEATHERBALL
  "거센 소용돌이 속에 상대를 4~5턴 동안 가둔다.",  // WHIRLPOOL
  "상대를 날려 교체시킨다. 야생 포켓몬이면 배틀이 끝난다.",  // WHIRLWIND
  "악의 기술을 극한까지 익힌 강렬한 일격. 반드시 급소에 맞는다.",  // WICKEDBLOW
  "번개 폭풍을 일으켜 바람과 번개로 공격한다. 마비시킬 때가 있다.",  // WILDBOLTSTORM
  "강한 전기를 두르고 부딪친다. 자신도 반동 데미지를 조금 입는다.",  // WILDCHARGE
  "섬뜩한 보랏빛 불꽃으로 상대에게 화상을 입힌다.",  // WILLOWISP
  "넓고 강한 날개로 친다.",  // WINGATTACK
  "단단한 껍질 속에 몸을 숨겨 방어를 올린다.",  // WITHDRAW
  "단단한 몸으로 부딪친다. 자신도 큰 반동 데미지를 입는다.",  // WOODHAMMER
  "자신의 공격과 특수공격을 올린다.",  // WORKUP
  "걱정을 일으키는 씨앗을 심어 상대의 특성을 불면으로 만든다.",  // WORRYSEED
  "덩굴이나 몸으로 상대를 4~5턴 동안 휘감아 조인다.",  // WRAP
  "세게 쥐어짠다. 상대의 HP가 많을수록 위력이 커진다.",  // WRINGOUT
  "낫이나 발톱을 가위처럼 교차시켜 벤다.",  // XSCISSOR
  "큰 하품으로 다음 턴에 상대를 잠들게 한다.",  // YAWN
  "전기 대포를 쏜다. 데미지를 주고 상대를 마비시킨다.",  // ZAPCANNON
  "염동력을 머리에 모아 박치기한다. 상대를 풀죽게 할 때가 있다.",  // ZENHEADBUTT
  "상대에게 부딪치며 강한 전기를 흘린다. 상대를 풀죽게 할 때가 있다.",  // ZINGZAP
  "상대는 지닌 도구를 쓸 수 없고 트레이너도 그 포켓몬에게 도구를 쓸 수 없다.",  // EMBARGO
  "5턴 동안 상대는 HP를 회복하는 기술, 특성, 도구를 쓸 수 없다.",  // HEALBLOCK
  "이 노래를 들은 포켓몬은 3턴 안에 교체하지 않으면 쓰러진다.",  // PERISHSONG
  "자신의 HP를 깎아 대신 공격을 받는 분신을 만든다.",  // SUBSTITUTE
  "5턴 동안 느린 포켓몬이 먼저 움직이는 이상한 공간을 만든다.",  // TRICKROOM
  "다음 턴에 최대 HP의 절반만큼 HP를 회복하는 소원을 빈다.",  // WISH
};

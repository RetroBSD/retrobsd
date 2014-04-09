/*
 * This source herein may be modified and/or distributed by anybody who
 * so desires, with the following restrictions:
 *    1.)  This notice shall not be removed.
 *    2.)  Credit shall not be taken for the creation of this source.
 *    3.)  This code is not to be traded, sold, or used for personal
 *         gain or profit.
 */
#define boolean int

#define NOTHING		((unsigned short)     0)
#define OBJECT		((unsigned short)    01)
#define MONSTER		((unsigned short)    02)
#define STAIRS		((unsigned short)    04)
#define HORWALL		((unsigned short)   010)
#define VERTWALL	((unsigned short)   020)
#define DOOR		((unsigned short)   040)
#define FLOOR		((unsigned short)  0100)
#define TUNNEL		((unsigned short)  0200)
#define TRAP		((unsigned short)  0400)
#define HIDDEN		((unsigned short) 01000)

#define ARMOR		((unsigned short)   01)
#define WEAPON		((unsigned short)   02)
#define SCROL		((unsigned short)   04)
#define POTION		((unsigned short)  010)
#define GOLD		((unsigned short)  020)
#define FOOD		((unsigned short)  040)
#define WAND		((unsigned short) 0100)
#define RING		((unsigned short) 0200)
#define AMULET		((unsigned short) 0400)
#define ALL_OBJECTS	((unsigned short) 0777)

#define LEATHER 0
#define RINGMAIL 1
#define SCALE 2
#define CHAIN 3
#define BANDED 4
#define SPLINT 5
#define PLATE 6
#define ARMORS 7

#define BOW 0
#define DART 1
#define ARROW 2
#define DAGGER 3
#define SHURIKEN 4
#define MACE 5
#define LONG_SWORD 6
#define TWO_HANDED_SWORD 7
#define WEAPONS 8

#define MAX_PACK_COUNT 24

#define PROTECT_ARMOR 0
#define HOLD_MONSTER 1
#define ENCH_WEAPON 2
#define ENCH_ARMOR 3
#define IDENTIFY 4
#define TELEPORT 5
#define SLEEP 6
#define SCARE_MONSTER 7
#define REMOVE_CURSE 8
#define CREATE_MONSTER 9
#define AGGRAVATE_MONSTER 10
#define MAGIC_MAPPING 11
#define CON_MON 12
#define SCROLS 13

#define INCREASE_STRENGTH 0
#define RESTORE_STRENGTH 1
#define HEALING 2
#define EXTRA_HEALING 3
#define POISON 4
#define RAISE_LEVEL 5
#define BLINDNESS 6
#define HALLUCINATION 7
#define DETECT_MONSTER 8
#define DETECT_OBJECTS 9
#define CONFUSION 10
#define LEVITATION 11
#define HASTE_SELF 12
#define SEE_INVISIBLE 13
#define POTIONS 14

#define TELE_AWAY 0
#define SLOW_MONSTER 1
#define INVISIBILITY 2
#define POLYMORPH 3
#define HASTE_MONSTER 4
#define MAGIC_MISSILE 5
#define CANCELLATION 6
#define DO_NOTHING 7
#define DRAIN_LIFE 8
#define COLD 9
#define FIRE 10
#define WANDS 11

#define STEALTH 0
#define R_TELEPORT 1
#define REGENERATION 2
#define SLOW_DIGEST 3
#define ADD_STRENGTH 4
#define SUSTAIN_STRENGTH 5
#define DEXTERITY 6
#define ADORNMENT 7
#define R_SEE_INVISIBLE 8
#define MAINTAIN_ARMOR 9
#define SEARCHING 10
#define RINGS 11

#define RATION 0
#define FRUIT 1

#define NOT_USED	((unsigned short)   0)
#define BEING_WIELDED	((unsigned short)  01)
#define BEING_WORN	((unsigned short)  02)
#define ON_LEFT_HAND	((unsigned short)  04)
#define ON_RIGHT_HAND	((unsigned short) 010)
#define ON_EITHER_HAND	((unsigned short) 014)
#define BEING_USED	((unsigned short) 017)

#define NO_TRAP -1
#define TRAP_DOOR 0
#define BEAR_TRAP 1
#define TELE_TRAP 2
#define DART_TRAP 3
#define SLEEPING_GAS_TRAP 4
#define RUST_TRAP 5
#define TRAPS 6

#define STEALTH_FACTOR 3
#define R_TELE_PERCENT 8

#define UNIDENTIFIED    ((unsigned short) 00)	/* MUST BE ZERO! */
#define IDENTIFIED      ((unsigned short) 01)
#define CALLED          ((unsigned short) 02)

#define DROWS 24
#define DCOLS 80
#define NMESSAGES 5
#define MAX_TITLE_LENGTH 30
#define MAXSYLLABLES 40
#define MAX_METAL 14
#define WAND_MATERIALS 30
#define GEMS 14

#define GOLD_PERCENT 46

#define SCORE_FILE "/games/lib/rogue.scores"
#define LOCK_FILE  "/games/lib/rogue.lock"

#define MAX_OPT_LEN 40

struct id {
	short value;
	char title[35];
	char *real;
	unsigned short id_status;
};

/* The following #defines provide more meaningful names for some of the
 * struct object fields that are used for monsters.  This, since each monster
 * and object (scrolls, potions, etc) are represented by a struct object.
 * Ideally, this should be handled by some kind of union structure.
 */

#define m_damage damage
#define hp_to_kill quantity
#define m_char ichar
#define first_level is_protected
#define last_level is_cursed
#define m_hit_chance class
#define stationary_damage identified
#define drop_percent which_kind
#define trail_char d_enchant
#define slowed_toggle quiver
#define moves_confused hit_enchant
#define nap_length picked_up
#define disguise what_is
#define next_monster next_object

struct obj {				/* comment is monster meaning */
	long m_flags;			/* monster flags */
	char *damage;			/* damage it does */
	short quantity;			/* hit points to kill */
	short ichar;			/* 'A' is for aquatar */
	short kill_exp;			/* exp for killing it */
	short is_protected;		/* level starts */
	short is_cursed;		/* level ends */
	short class;			/* chance of hitting you */
	short identified;		/* 'F' damage, 1,2,3... */
	unsigned short which_kind;      /* item carry/drop % */
	short o_row, o_col, o;          /* o is how many times stuck at o_row, o_col */
	short row, col;			/* current row, col */
	short d_enchant;		/* room char when detect_monster */
	short quiver;			/* monster slowed toggle */
	short trow, tcol;		/* target row, col */
	short hit_enchant;		/* how many moves is confused */
	unsigned short what_is;         /* imitator's charactor (?!%: */
	short picked_up;		/* sleep from wand of sleep */
	unsigned short in_use_flags;
	struct obj *next_object;	/* next monster */
};

typedef struct obj object;

#define INIT_AW (object*)0,(object*)0
#define INIT_RINGS (object*)0,(object*)0
#define INIT_HP 12,12
#define INIT_STR 16,16
#define INIT_EXP 1,0
#define INIT_PACK {0}
#define INIT_GOLD 0
#define INIT_CHAR '@'
#define INIT_MOVES 1250

struct fightr {
	object *armor;
	object *weapon;
	object *left_ring, *right_ring;
	short hp_current;
	short hp_max;
	short str_current;
	short str_max;
	object pack;
	long gold;
	short exp;
	long exp_points;
	short row, col;
	short fchar;
	short moves_left;
};

typedef struct fightr fighter;

struct dr {
	short oth_room;
	short oth_row,
	      oth_col;
	short door_row,
              door_col;
};

typedef struct dr door;

struct rm {
	short bottom_row, right_col, left_col, top_row;
	door doors[4];
	unsigned short is_room;
};

typedef struct rm room;

#define MAXROOMS 9
#define BIG_ROOM 10

#define NO_ROOM -1

#define PASSAGE -3		/* cur_room value */

#define AMULET_LEVEL 26

#define R_NOTHING	((unsigned short) 01)
#define R_ROOM		((unsigned short) 02)
#define R_MAZE		((unsigned short) 04)
#define R_DEADEND	((unsigned short) 010)
#define R_CROSS		((unsigned short) 020)

#define MAX_EXP_LEVEL 21
#define MAX_EXP 10000001L
#define MAX_GOLD 999999
#define MAX_ARMOR 99
#define MAX_HP 999
#define MAX_STRENGTH 99
#define LAST_DUNGEON 99

#define STAT_LEVEL 01
#define STAT_GOLD 02
#define STAT_HP 04
#define STAT_STRENGTH 010
#define STAT_ARMOR 020
#define STAT_EXP 040
#define STAT_HUNGER 0100
#define STAT_LABEL 0200
#define STAT_ALL 0377

#define PARTY_TIME 10	/* one party somewhere in each 10 level span */

#define MAX_TRAPS 10	/* maximum traps per level */

#define HIDE_PERCENT 12

struct tr {
	short trap_type;
	short trap_row, trap_col;
};

typedef struct tr trap;

extern fighter rogue;
extern room rooms[];
extern trap traps[];
extern unsigned short dungeon[DROWS][DCOLS];
extern object level_objects;

extern struct id id_scrolls[];
extern struct id id_potions[];
extern struct id id_wands[];
extern struct id id_rings[];
extern struct id id_weapons[];
extern struct id id_armors[];

extern object mon_tab[];
extern object level_monsters;

#define MONSTERS 26

#define HASTED			01L
#define SLOWED			02L
#define INVISIBLE		04L
#define ASLEEP                 010L
#define WAKENS                 020L
#define WANDERS                040L
#define FLIES                 0100L
#define FLITS                 0200L
#define CAN_FLIT	      0400L	/* can, but usually doesn't, flit */
#define CONFUSED	     01000L
#define RUSTS		     02000L
#define HOLDS		     04000L
#define FREEZES		    010000L
#define STEALS_GOLD         020000L
#define STEALS_ITEM	    040000L
#define STINGS		   0100000L
#define DRAINS_LIFE	   0200000L
#define DROPS_LEVEL	   0400000L
#define SEEKS_GOLD	  01000000L
#define FREEZING_ROGUE	  02000000L
#define RUST_VANISHED	  04000000L
#define CONFUSES	 010000000L
#define IMITATES	 020000000L
#define FLAMES		 040000000L
#define STATIONARY	0100000000L	/* damage will be 1,2,3,... */
#define NAPPING		0200000000L	/* can't wake up for a while */
#define ALREADY_MOVED	0400000000L

#define SPECIAL_HIT	(RUSTS|HOLDS|FREEZES|STEALS_GOLD|STEALS_ITEM|STINGS|DRAINS_LIFE|DROPS_LEVEL)

#define WAKE_PERCENT 45
#define FLIT_PERCENT 40
#define PARTY_WAKE_PERCENT 75

#define HYPOTHERMIA 1
#define STARVATION 2
#define POISON_DART 3
#define QUIT 4
#define WIN 5
#define KFIRE 6

#define UPWARD 0
#define UPRIGHT 1
#define RIGHT 2
#define DOWNRIGHT 3
#define DOWN 4
#define DOWNLEFT 5
#define LEFT 6
#define UPLEFT 7
#define DIRS 8

#define ROW1 7
#define ROW2 15

#define COL1 26
#define COL2 52

#define MOVED 0
#define MOVE_FAILED -1
#define STOPPED_ON_SOMETHING -2
#define CANCEL '\033'
#define LIST '*'

#define HUNGRY 300
#define WEAK 150
#define FAINT 20
#define STARVE 0

#define MIN_ROW 1

struct rogue_time {
	short year;	/* >= 1987 */
	short month;	/* 1 - 12 */
	short day;	/* 1 - 31 */
	short hour;	/* 0 - 23 */
	short minute;	/* 0 - 59 */
	short second;	/* 0 - 59 */
};

#ifdef CURSES
struct _win_st {
	short _cury, _curx;
	short _maxy, _maxx;
};

typedef struct _win_st WINDOW;

extern int LINES, COLS;
extern WINDOW *curscr;
extern char *CL;

void initscr(void);
void endwin(void);
void noecho(void);
void crmode(void);
void nonl(void);
void refresh(void);
void wrefresh(WINDOW *scr);
void clear(void);
void clrtoeol(void);
void tstp(void);
void standout(void);
void standend(void);
void addstr(char *str);
void addch(int ch);
void move(int row, int col);
void mvaddch(int row, int col, int ch);
void mvaddstr(int row, int col, char *str);
int mvinch(int row, int col);

#else
#include <curses.h>
#endif

#ifdef CROSS
#   include </usr/include/stdio.h>
#else
#   include <stdio.h>
#endif

extern char ask_quit;
extern char being_held;
extern char cant_int;
extern char con_mon;
extern char detect_monster;
extern char did_int;
extern char interrupted;
extern char jump;
extern char maintain_armor;
extern char mon_disappeared;
extern char msg_cleared;
extern char no_skull;
extern char passgo;
extern char r_see_invisible;
extern char r_teleport;
extern char save_is_interactive;
extern char score_only;
extern char see_invisible;
extern char sustain_strength;
extern char trap_door;
extern char wizard;
extern char *byebye_string;
extern char *curse_message;
extern char *error_file;
extern char *fruit;
extern char *m_names[];
extern char *more;
extern char *new_level_message;
extern char *nick_name;
extern char *press_space;
extern char *save_file;
extern char *you_can_move_again;
extern char hit_message[];
extern char hunger_str[];
extern char login_name[];
extern long level_points[];
extern char is_wood[];
extern short add_strength;
extern short auto_search;
extern short bear_trap;
extern short blind;
extern short confused;
extern short cur_level;
extern short cur_room;
extern short e_rings;
extern short extra_hp;
extern short foods;
extern short halluc;
extern short haste_self;
extern short less_hp;
extern short levitate;
extern short max_level;
extern short m_moves;
extern short party_room;
extern short regeneration;
extern short ring_exp;
extern short r_rings;
extern short stealthy;

void rand_around(int i, int *r, int *c);
void place_at(object *obj, int row, int col);
void message(char *msg, boolean intrpt);
void sound_bell(void);
void check_message(void);
void unwield(object *obj);
void mv_aquatars(void);
void unwear(object *obj);
void print_stats(int stat_mask);
void un_put_on(object *ring);
void wake_up(object *monster);
void check_gold_seeker(object *monster);
void s_con_mon(object *monster);
void vanish(object *obj, int rm, object *pack);
void get_dir_rc(int dir, int *row, int *col, int allow_off_screen);
void rogue_damage(int d, object *monster, int other);
void mv_1_monster(object *monster, int row, int col);
void move_mon_to(object *monster, int row, int col);
void do_wear(object *obj);
void do_wield(object *obj);
void do_put_on(object *ring, boolean on_left);
void inv_rings(void);
void ring_stats(boolean pr);
void rust(object *monster);
void tele(void);
void unhallucinate(void);
void unblind(void);
void relight(void);
void take_a_nap(void);
void cnfs(void);
void unconfuse(void);
void special_hit(object *monster);
void killed_by(object *monster, int other);
void cough_up(object *monster);
void add_exp(int e, boolean promotion);
void take_from_pack(object *obj, object *pack);
void free_object(object *obj);
void put_amulet(void);
void md_cbreak_no_echo_nonl(boolean on);
void md_tstp(void);
void md_slurp(void);
void start_window(void);
void stop_window(void);
void gr_row_col(int *row, int *col, unsigned mask);
void light_up_room(int rn);
void light_passage(int row, int col);
void wake_room(int rn, boolean entering, int row, int col);
void win(void);
void clean_up(char *estr);
void multiple_move_rogue(int dirch);
void get_food(object *obj, boolean force_ration);
void put_scores(object *monster, int other);
void xxxx(char *buf, int n);
void md_heed_signals(void);
void srrandom(int x);
void restore(char *fname);
void mix_colors(void);
void get_wand_and_ring_materials(void);
void make_scroll_titles(void);
void md_exit(int status);
void md_control_keybord(boolean mode);
void md_ignore_signals(void);
void quit(boolean from_intrpt);
void save_into_file(char *sfile);
void get_desc(object *obj, char *desc);
void wait_for_ack(void);
void make_level(void);
void clear_level(void);
void put_objects(void);
void put_stairs(void);
void add_traps(void);
void put_mons(void);
void put_player(int nr);
void play_level(void);
void free_stuff(object *objlist);
void mon_hit(object *monster);
void dr_course(object *monster, boolean entering, int row, int col);
void rogue_hit(object *monster, boolean force_hit);
void darken_room(int rn);
void trap_player(int row, int col);
void mv_mons(void);
void wanderer(void);
void hallucinate(void);
void search(int n, boolean is_auto);
void party_monsters(int rn, int n);
void gr_ring(object *ring, boolean assign_wk);
void inventory(object *pack, unsigned mask);
void rest(int count);
void fight(boolean to_the_death);
void eat(void);
void quaff(void);
void read_scroll(void);
void move_onto(void);
void kick_into_pack(void);
void drop(void);
void put_on_ring(void);
void remove_ring(void);
void remessage(int c);
void wizardize(void);
void inv_armor_weapon(boolean is_weapon);
void id_trap(void);
void id_type(void);
void id_com(void);
void do_shell(void);
void edit_opts(void);
void single_inv(int ichar);
void take_off(void);
void wear(void);
void wield(void);
void call_it(void);
void zapp(void);
void throw(void);
void draw_magic_map(void);
void show_traps(void);
void show_objects(void);
void show_average_hp(void);
void c_object_for_wizard(void);
void show_monsters(void);
void save_game(void);
void md_shell(char *shell);
void md_gct(struct rogue_time *rt_buf);
void md_gfmt(char *fname, struct rogue_time *rt_buf);
void md_lock(boolean l);
void bounce(int ball, int dir, int row, int col, int r);
void create_monster(void);
void aggravate(void);
void md_sleep(int nsecs);
void onintr(int sig);
void byebye(int sig);
void error_save(int sig);

int rogue_can_see(int row, int col);
int get_dungeon_char(int row, int col);
int rgetchar(void);
int pack_letter(char *prompt, unsigned mask);
int get_hit_chance(object *weapon);
int get_weapon_damage(object *weapon);
int rand_percent(int percentage);
int get_rand(int x, int y);
int mon_damage(object *monster, int damage);
int get_mask_char(unsigned mask);
int imitating(int row, int col);
int get_damage(char *ds, boolean r);
int get_armor_class(object *obj);
int check_imitator(object *monster);
int get_number(char *s);
int can_move(int row1, int col1, int row2, int col2);
int one_move_rogue(int dirch, int pickup);
int is_all_connected(void);
int coin_toss(void);
int has_amulet(void);
int get_room_number(int row, int col);
int hp_raise(void);
int md_gseed(void);
int is_vowel(int ch);
int init(int argc, char **argv);
int gr_obj_char(void);
int mon_can_go(object *monster, int row, int col);
int m_confuse(object *monster);
int flame_broil(object *monster);
int seek_gold(object *monster);
int is_passable(int row, int col);
int gr_room(void);
int party_objects(int rn);
int pack_count(object *new_obj);
int r_index(char *str, int ch, boolean last);
int get_input_line(char *prompt, char *insert, char *buf, char *if_cancelled,
                   boolean add_blank, boolean do_echo);
int drop_check(void);
int check_up(void);
int gmc_row_col(int row, int col);
int md_get_file_id(char *fname);
int md_link_count(char *fname);
int gmc(object *monster);

long xxx(boolean st);
long lget_number(char *s);

boolean is_direction(int c, int *d);
boolean mon_sees(object *monster, int row, int col);
boolean reg_move(void);
boolean is_digit(int ch);
boolean md_df(char *fname);

char *md_getenv(char *name);
char *md_malloc(int n);
char *md_gln(void);
char *mon_name(object *monster);
char *name_of(object *obj);

object *alloc_object(void);
object *object_at(object *pack, int row, int col);
object *add_to_pack(object *obj, object *pack, int condense);
object *get_letter_object(int ch);
object *gr_object(void);
object *gr_monster(object *monster, int mn);
object *pick_up(int row, int col, int *status);

struct id *get_id_table(object *obj);

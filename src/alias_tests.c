#include <stdlib.h>
#include <string.h>

#include "alias.h"
#include "error.h"
#include "unit.h"

DEFTEST("alias.init")
{
	struct alias_table *aliases = alias_table_new();
	ASSERT_NOT_NULL(aliases);
	alias_table_free(aliases);
}

DEFTEST("alias.simple")
{
	struct alias_table *aliases = alias_table_new();
	ASSERT_NOT_NULL(aliases);

	alias_set(aliases, "foo", "bar");
	EXPECT(!strcmp(alias_get(aliases, "foo"), "bar"));

	alias_table_free(aliases);
}

DEFTEST("alias.simple_two")
{
	struct alias_table *aliases = alias_table_new();
	ASSERT_NOT_NULL(aliases);

	alias_set(aliases, "goodies", "candy");
	alias_set(aliases, "yummies", "sugar");

	EXPECT(!strcmp(alias_get(aliases, "goodies"), "candy"));
	EXPECT(!strcmp(alias_get(aliases, "yummies"), "sugar"));

	alias_table_free(aliases);
}

DEFTEST("alias.get_unknown_without_values")
{
	struct alias_table *aliases = alias_table_new();
	ASSERT_NOT_NULL(aliases);

	EXPECT_NULL(alias_get(aliases, "randomthing"));

	alias_table_free(aliases);
}

DEFTEST("alias.get_unknown_with_values")
{
	struct alias_table *aliases = alias_table_new();
	ASSERT_NOT_NULL(aliases);

	alias_set(aliases, "serpent", "snake");
	alias_set(aliases, "lizard", "gecko");

	EXPECT_NULL(alias_get(aliases, "iguana"));

	alias_table_free(aliases);
}

DEFTEST("alias.different_pointers")
{
	struct alias_table *aliases = alias_table_new();
	ASSERT_NOT_NULL(aliases);

	const char name[] = "ImaDifferentPointer";
	char copy[sizeof(name)];
	memcpy(copy, name, sizeof(name));

	const char value[] = "%%%%ddddgaAaaAaAahzzzzzzzzp";

	alias_set(aliases, name, value);

	EXPECT(!strcmp(alias_get(aliases, name), value));
	EXPECT(!strcmp(alias_get(aliases, copy), value));

	alias_table_free(aliases);
}

DEFTEST("alias.prefix_no_match")
{
	struct alias_table *aliases = alias_table_new();
	ASSERT_NOT_NULL(aliases);

	const char name1[] = "BlackJack";
	const char name2[] = "Black";

	const char value1[] = "TwentyOne";
	const char value2[] = "TheDarkestColor";

	alias_set(aliases, name1, value1);
	alias_set(aliases, name2, value2);

	EXPECT(!strcmp(alias_get(aliases, name1), value1));
	EXPECT(!strcmp(alias_get(aliases, name2), value2));

	alias_table_free(aliases);
}

DEFTEST("alias.change_definition")
{
	struct alias_table *aliases = alias_table_new();
	ASSERT_NOT_NULL(aliases);

	const char name[] = "duck";

	const char value1[] = "quack";
	const char value2[] = "waddle";

	alias_set(aliases, name, value1);
	EXPECT(!strcmp(alias_get(aliases, name), value1));

	alias_set(aliases, name, value2);
	EXPECT(!strcmp(alias_get(aliases, name), value2));

	alias_set(aliases, name, value1);
	EXPECT(!strcmp(alias_get(aliases, name), value1));

	alias_table_free(aliases);
}

DEFTEST("alias.mutate_data")
{
	struct alias_table *aliases = alias_table_new();
	ASSERT_NOT_NULL(aliases);

	const char name[] = "gazebo";
	const char value[] = "attack";

	char namecpy[sizeof(name)];
	memcpy(namecpy, name, sizeof(name));

	char valuecpy[sizeof(value)];
	memcpy(valuecpy, value, sizeof(value));

	alias_set(aliases, namecpy, valuecpy);
	EXPECT(!strcmp(alias_get(aliases, name), value));
	namecpy[0] = 'b';
	valuecpy[4] = 'g';
	EXPECT(!strcmp(alias_get(aliases, name), value));

	alias_table_free(aliases);
}

DEFTEST("alias.complete_get_set")
{
	struct alias_table *aliases = alias_table_new();
	ASSERT_NOT_NULL(aliases);

	struct {
		const char *name;
		const char *value;
	} aliasdefs[] = {
		{ "JackOfClubs", "Pickup5" },
		{ "JackOf", "What" },
		{ "KingOfHearts", "HasABrokenHeart" },
		{ "KingOfDiamonds", "IsQuiteRich" },
		{ "%%%%", "thatsfour" },
		{ "%%%%%", "thatsfive" },
		{ "%%%", "thatsthree" },
		{ "ditto", "same" },
		{ "dittoo", "same" },
		{ "++++", "plus" },
		{ "plus", "minus" },
		{ "a", "eh" },
		{ "alias", "nickname" },
		{ "nickname", "alias" },
		{ "aaargh", "rrrr" },
		{ "dashes", "---------------------------------DDDDDASH" },
		{ "%%%%%%%%%", "thatsnine" },
		{ "%%%%%%%%", "thatseight" },
		{ "%%%%%%%", "thatsseven" },
		{ "%%%%%%", "thatssix" },
		{ "bbb", "ccc" },
		{ "(((())))", "[[[[]]]]" },
		{ "-", "@" },
		{ "--", "@" },
	};

	for (size_t i = 0; i < ARRAY_SIZE(aliasdefs); i++)
		alias_set(aliases, aliasdefs[i].name, aliasdefs[i].value);
	for (size_t i = 0; i < ARRAY_SIZE(aliasdefs); i++)
		EXPECT(!strcmp(alias_get(aliases, aliasdefs[i].name),
			       aliasdefs[i].value));

	alias_table_free(aliases);
}

DEFTEST("alias.unset.simple")
{
	struct alias_table *aliases = alias_table_new();
	ASSERT_NOT_NULL(aliases);

	alias_set(aliases, "flip", "flop");
	EXPECT(!strcmp(alias_get(aliases, "flip"), "flop"));
	alias_unset(aliases, "flip");
	EXPECT_NULL(alias_get(aliases, "flip"));

	alias_table_free(aliases);
}

DEFTEST("alias.unset.middle")
{
	struct alias_table *aliases = alias_table_new();
	ASSERT_NOT_NULL(aliases);

	alias_set(aliases, "flip", "flop");
	alias_set(aliases, "bip", "bop");
	alias_set(aliases, "tick", "tock");
	alias_set(aliases, "zip", "zop");

	EXPECT(!strcmp(alias_get(aliases, "flip"), "flop"));
	EXPECT(!strcmp(alias_get(aliases, "bip"), "bop"));
	EXPECT(!strcmp(alias_get(aliases, "tick"), "tock"));
	EXPECT(!strcmp(alias_get(aliases, "zip"), "zop"));

	alias_unset(aliases, "tick");
	EXPECT_NULL(alias_get(aliases, "tick"));

	EXPECT(!strcmp(alias_get(aliases, "flip"), "flop"));
	EXPECT(!strcmp(alias_get(aliases, "bip"), "bop"));
	EXPECT(!strcmp(alias_get(aliases, "zip"), "zop"));

	alias_table_free(aliases);
}

DEFTEST("alias.unset.nodef")
{
	struct alias_table *aliases = alias_table_new();
	ASSERT_NOT_NULL(aliases);

	alias_unset(aliases, "flip");

	alias_set(aliases, "flip", "flop");
	alias_set(aliases, "bip", "bop");
	alias_set(aliases, "tick", "tock");
	alias_set(aliases, "zip", "zop");

	alias_unset(aliases, "im_not_here");

	EXPECT(!strcmp(alias_get(aliases, "flip"), "flop"));
	EXPECT(!strcmp(alias_get(aliases, "bip"), "bop"));
	EXPECT(!strcmp(alias_get(aliases, "tick"), "tock"));
	EXPECT(!strcmp(alias_get(aliases, "zip"), "zop"));

	alias_table_free(aliases);
}

/*
 * Copyright (c) 1983 Regents of the University of California,
 * All rights reserved.  Redistribution permitted subject to
 * the terms of the Berkeley Software License Agreement.
 */
#include "externs.h"

#define strstr(x) x

struct room dayfile[] = {
	{ 0 },
	{ strstr("You are in the main hangar."),
                { 5, 2, 9, 3, 3, 1, 0, 0 },
strstr("This is a huge bay where many fighters and cargo craft lie.  Alarms are \n\
sounding and fighter pilots are running to their ships.  Above is a gallery\n\
overlooking the bay. The scream of turbo engines is coming from +. The rest\n\
of the hangar is +. There is an exit +.*\n") },
	{ strstr("This is the landing bay."),
	        { 1, 0, 10, 0, 0, 0, 0, 0 },
strstr("Ships are landing here, some heavily damaged. Enemy fighters continually\n\
strafe this vulnerable port. The main hangar is +, *\n\
There is an exit +.*\n") },
	{ strstr("You are in the gallery."),
	        { 4, 0, 0, 0, 0, 0, 1, 0 },
strstr("From here a view of the entire landing bay reveals that our battlestar\n\
is near destruction. Fires are spreading out of control and laser blasts\n\
lick at the shadows. The control room is +. ***\n") },
	{ strstr("You are in the control room."),
	        { 0, 3, 0, 0, 0, 0, 5, 0 },
strstr("Several frantic technicians are flipping switches wildly but otherwise\n\
this room seems fairly deserted.  A weapons locker has been left open.\n\
A staircase leads down. * There is a way -. **        \n") },
	{ strstr("This is the launch room."),
	        { 6, 1, 7, 0, 4, 1, 0, 0 },
strstr("From the launch tubes here fighters blast off into space. Only one is left,\n\
and it is guarded by two fierce men. A staircase leads up from here.\n\
There is a cluttered workbench +. From the main hangar come sounds of great\n\
explosions.  The main hangar is +. The viper launch tubes are to the -.*\n") },
	{ strstr("You are at the workbench."),
	        { 0, 5, 7, 0, 0, 0, 0, 0 },
strstr("Strange and unwieldy tools are arranged here including a lunch box \n\
and pneumatic wrenches and turbo sprocket rockets.*\n\
The launch room is +. The remaining viper is +.*\n") },
	{ strstr("You are in the viper launch tube."),
	        { 0, 5, 0, 5, 32, 0, 0, 0 },
strstr("The two guards are eyeing you warily! ****\n") },
	{ strstr("This is a walk in closet."),
	        { 22, 0, 0, 0, 0, 0, 0, 0 },
strstr("A wardrobe of immense magnitude greets the eye.  Furs and robes of kings\n\
hang on rack after rack.  Silken gowns, capes woven with spun gold, and \n\
delicate synthetic fabrics are stowed here.  The bedroom is +.***\n") },
	{ strstr("You are in a wide hallway leading to the main hangar."),
	        { 0, 0, 11, 1, 0, 0, 0, 0 },
strstr("The walls and ceiling here have been blasted through in several places.\n\
It looks as if quite a battle has been fought for possession of the landing bay\n\
Gaping corpses litter the floor.**  The hallway continues +.\n\
The main hangar is +.\n") },
	{ strstr("You are in a wide hallway leading to the landing bay."),
	        { 0, 0, 12, 2, 0, 0, 0, 0 },
strstr("Most of the men and supplies needed in the main hangar come through this\n\
corridor, but the wounded are forced to use it too. It very dank and\n\
crowded here, and the floor is slippery with blood.**\n\
The hallway continues -. The landing bay is +.\n") },
	{ strstr("The hallway is very congested with rubble here."),
	        { 0, 0, 0, 9, 13, 1, 0, 0 },
strstr("It is too choked with broken steel girders and other debris to continue\n\
on much farther. Above, the ceiling has caved in and it is possible to \n\
climb up. There is not much chance to go -, -, or -.\n\
But the hallway seems clearer +.\n") },
	{ strstr("A wide hallway and a more narrow walkway meet here."),
	        { 14, 15, 0, 10, 0, 0, 0, 0 },
strstr("The intersection is crowded with the many wounded who have come up\n\
the wide hallway and continued +. The walkway is less crowded +.\n\
The wide hallway goes *-.\n") },
	{ strstr("You are in what was once an elegant stateroom."),
	        { 16, 0, 0, 0, 0, 0, 11, 0 },
strstr("Whoever lived in this stateroom, he and his female companion\n\
were mercilessly slain in their sleep. Clothes, trinkets and personal\n\
belongings are scattered all across the floor. Through a hole in the\n\
collapsed floor I can see a hallway below.  A door is +.***\n") },
	{ strstr("You're at the entrance to the sick bay."),
	        { 17, 12, 18, 0, 0, 0, 0, 0 },
strstr("The wounded are entering the sick bay in loudly moaning files.\n\
The walkway continues - and +. A doctor is motioning for you to \n\
come to the -. *\n") },
	{ strstr("You're in the walkway."),
	        { 12, 19, 0, 0, 0, 0, 0, 0 },
strstr("Most of the men and supplies were coming from the armory. The walkway\n\
continues -. The armory is +.**\n") },
	{ strstr("These are the executive suites of the battlestar."),
	        { 20, 13, 21, 22, 23, 1, 24, 0 },
strstr("Luxurious staterooms carpeted with crushed velvet and adorned with beaten\n\
gold open onto this parlor. A wide staircase with ivory banisters leads\n\
up or down. This parlor leads into a hallway +. The bridal suite is +.\n\
Other rooms lie - and +.\n") },
	{ strstr("You're in a long dimly lit hallway."),
	        { 0, 14, 25, 0, 0, 0, 0, 0 },
strstr("This part of the walkway is deserted. There is a dead end +. The\n\
entrance to the sickbay is +. The walkway turns sharply -.*\n") },
	{ strstr("This is the sick bay."),
	        { 0, 0, 0, 14, 0, 0, 0, 0 },
strstr("Sinister nurses with long needles and pitiful aim probe the depths of suffering\n\
here. Only the mortally wounded receive medical attention on a battlestar,\n\
but afterwards they are thrown into the incinerators along with the rest.**\n\
Nothing but death and suffering +.  The walkway is +.\n") },
	{ strstr("You're in the armory."),
	        { 15, 26, 0, 0, 0, 0, 0, 0 },
strstr("An armed guard is stationed here 365 sectars a yarn to protect the magazine.\n\
The walkway is +. The magazine is +.**\n") },
	{ strstr("The hallway ends here at the presidential suite."),
	        { 27, 16, 0, 0, 0, 0, 0, 0 },
strstr("The door to this suite is made from solid magnesium, and the entryway is\n\
inlaid with diamonds and fire opals. The door is ajar +. The hallway\n\
goes -.**\n") },
	{ strstr("This is the maid's utility room."),
	        { 0, 0, 0, 16, 0, 0, 0, 0 },
strstr("What a gruesome sight! The maid has been brutally drowned in a bucket of\n\
Pine Sol and repeatedly stabbed in the back with a knife.***\n\
The hallway is +.\n") },
	{ strstr("This is a luxurious stateroom."),
	        { 0, 8, 16, 0, 0, 0, 0, 0 },
strstr("The floor is carpeted with a soft animal fur and the great wooden furniture\n\
is inlaid with strips of platinum and gold.  Electronic equipment built\n\
into the walls and ceiling is flashing wildly.  The floor shudders and\n\
the sounds of dull explosions rumble though the room.  From a window in\n\
the wall + comes a view of darkest space.  There is a small adjoining\n\
room +, and a doorway +.*\n") },
	{ strstr("You are at the entrance to the dining hall."),
	        { 0, 0, 28, 0, 0, 0, 16, 0 },
strstr("A wide staircase with ebony banisters leads down here.**\n\
The dining hall is to the -.*\n") },
	{ strstr("This was once the first class lounge."),
	        { 0, 0, 29, 0, 16, 1, 0, 0 },
strstr("There is much rubble and destruction here that was not apparent elsewhere.\n\
The walls and ceilings have broken in in some places. A staircase with\n\
red coral banisters leads up. It is impossible to go - or -.\n\
It seems a little clearer +.*\n") },
	{ strstr("You are in a narrow stairwell."),
	        { 0, 17, 0, 0, 30, 1, 0, 0 },
strstr("These dusty and decrepit stairs lead up.  There is no way -.  The\n\
hallway turns sharply -.**\n") },
	{ strstr("You are in the magazine."),
	        { 19, 0, 0, 0, 0, 0, 0, 0 },
strstr("Rows and rows of neatly stacked ammunition for laser pistols and grenade\n\
launchers are here. The armory is +.***\n") },
	{ strstr("You're in the presidential suite."),
	        { 0, 20, 0, 0, 0, 0, 0, 0 },
strstr("Apparently the president has been assassinated. A scorched figure lies\n\
face downward on the carpet clutching his chest.*\n\
The hallway leads -.**\n") },
	{ strstr("You are in the dining hall."),
	        { 0, 30, 31, 23, 0, 0, 0, 0 },
strstr("This was the seen of a mass suicide. Hundreds of ambassadors and assorted\n\
dignitaries sit slumped over their breakfast cereal. I suppose the news\n\
of the cylon attack killed them. There is a strange chill in this room.  I\n\
would not linger here. * The kitchen is +. Entrances + and +.\n") },
	{ strstr("The debris is very thick here."),
	        { 0, 11, 0, 24, 0, 0, 0, 0 },
strstr("Broken furniture, fallen girders, and other rubble block the way.\n\
There is not much chance to continue -, -, or -.\n\
It would be best to go -.\n") },
	{ strstr("You are in the kitchen."),
	        { 28, 0, 0, 0, 0, 0, 0, 0 },
strstr("This room is full of shining stainless steel and burnished bronze cookware. An \n\
assortment of tropical fruits and vegetables as well as fine meats and cheeses \n\
lies on a sterling platter. The chef, unfortunately, has been skewered like a \n\
side of beef. The dining room is +. ** There is a locked door +.\n") },
	{ strstr("You are in an arched entry leading to the dining room."),
	        { 0, 0, 0, 28, 0, 0, 0, 0 },
strstr("The door leading out is bolted shut from the outside and is very strong.***\n\
The dining room is +.\n") },
	{ strstr("You are in space."),
	        { 33, 34, 35, 36, 37, 1, 33, 1 },
strstr("****\n") },
	{ strstr("You are in space."),
	        { 38, 32, 39, 40, 41, 1, 42, 1 },
strstr("****\n") },
	{ strstr("You are in space."),
	        { 32, 44, 45, 46, 47, 1, 48, 1 },
strstr("****\n") },
	{ strstr("You are in space."),
	        { 40, 45, 49, 32, 50, 1, 51, 1 },
strstr("****\n") },
	{ strstr("You are in space."),
	        { 41, 46, 32, 52, 53, 1, 54, 1 },
strstr("****\n") },
	{ strstr("You are in space."),
	        { 42, 47, 50, 53, 55, 1, 32, 1 },
strstr("****\n") },
	{ strstr("You are in space."),
	        { 43, 48, 51, 54, 32, 1, 56, 1 },
strstr("****\n") },
	{ strstr("You are in space."),
	        { 57, 33, 40, 41, 42, 1, 43, 1 },
strstr("****\n") },
	{ strstr("You are in space."),
	        { 39, 35, 57, 33, 58, 1, 59, 1 },
strstr("****\n") },
	{ strstr("You are in space."),
	        { 39, 36, 33, 59, 60, 1, 61, 1 },
strstr("****\n") },
	{ strstr("You are in space."),
	        { 39, 37, 58, 60, 62, 1, 33, 1 },
strstr("****\n") },
	{ strstr("You are in space."),
	        { 39, 38, 59, 61, 33, 1, 63, 1 },
strstr("****\n") },
	{ strstr("You are in space."),
	        { 34, 64, 45, 46, 47, 1, 48, 1 },
strstr("****\n") },
	{ strstr("You are in space."),
	        { 35, 44, 49, 34, 50, 1, 51, 1 },
strstr("****\n") },
	{ strstr("You are in space."),
	        { 36, 44, 34, 52, 53, 1, 54, 1 },
strstr("****\n") },
	{ strstr("You are in space."),
	        { 37, 44, 50, 53, 55, 1, 34, 1 },
strstr("****\n") },
	{ strstr("You are in space."),
	        { 38, 44, 51, 54, 34, 1, 56, 1 },
strstr("****\n") },
	{ strstr("You are in space."),
	        { 49, 49, 52, 35, 49, 1, 49, 1 },
strstr("****\n") },
	{ strstr("You are in space."),
	        { 58, 47, 49, 37, 55, 1, 35, 1 },
strstr("****\n") },
	{ strstr("You are in space."),
	        { 59, 48, 49, 38, 35, 1, 56, 1 },
strstr("****\n") },
	{ strstr("You are in space."),
	        { 52, 52, 36, 49, 52, 1, 52, 1 },
strstr("****\n") },
	{ strstr("You are in space."),
	        { 60, 46, 37, 52, 55, 1, 36, 1 },
strstr("****\n") },
	{ strstr("You are in space."),
	        { 61, 48, 38, 52, 36, 1, 56, 1 },
strstr("****\n") },
	{ strstr("You are in space."),
	        { 62, 55, 55, 55, 56, 1, 37, 1 },
strstr("****\n") },
	{ strstr("You are in space."),
	        { 56, 56, 56, 56, 38, 1, 55, 1 },
strstr("****\n") },
	{ strstr("You are in space."),
	        { 65, 39, 57, 57, 57, 1, 57, 1 },
strstr("****\n") },
	{ strstr("You are in space."),
	        { 39, 50, 49, 42, 62, 1, 40, 1 },
strstr("****\n") },
	{ strstr("You are in space."),
	        { 39, 51, 49, 43, 40, 1, 63, 1 },
strstr("****\n") },
	{ strstr("You are in space."),
	        { 39, 53, 43, 59, 62, 1, 41, 1 },
strstr("****\n") },
	{ strstr("You are in space."),
	        { 39, 54, 43, 59, 41, 1, 56, 1 },
strstr("****\n") },
	{ strstr("You are in space."),
	        { 39, 55, 62, 62, 56, 1, 42, 1 },
strstr("****\n") },
	{ strstr("You are in space."),
	        { 39, 56, 35, 36, 43, 1, 55, 1 },
strstr("****\n") },
	{ strstr("You are in space."),
	        { 44, 66, 66, 66, 66, 1, 66, 1 },
strstr("****\n") },
	{ strstr("You are in space."),
	        { 67, 57, 67, 67, 67, 1, 67, 1 },
strstr("****\n") },
	{ strstr("You are in space."),
	        { 64, 68, 68, 68, 68, 1, 68, 1 },
strstr("****\n") },
	{ strstr("You are orbiting a small blue planet."),
	        { 67, 67, 67, 67, 65, 1, 69, 1 },
strstr("****\n") },
	{ strstr("You are orbiting a tropical planet."),
	        { 68, 68, 68, 68, 66, 1, 70, 1 },
strstr("****\n") },
	{ strstr("You are flying through a dense fog."),
	        { 69, 69, 69, 69, 69, 1, 69, 1 },
strstr("A cold grey sea of mist is swirling around the windshield and water droplets\n\
are spewing from the wingtips. Ominous shadows loom in the darkness and it\n\
feels as if a trap is closing around us. I have lost all sense of direction.\n\
****\n") },
	{ strstr("You are approaching an island."),
	        { 71, 72, 73, 74, 68, 1, 0, 1 },
strstr("Coconut palms, sword ferns, orchids, and other lush vegetation drape this\n\
jagged island carved seemingly from pure emerald and set in a turquoise\n\
sea. The land rises sharply +. There is a nice beach* +.*\n") },
	{ strstr("You are flying over a mountainous region."),
	        { 75, 73, 76, 77, 68, 1, 0, 1 },
strstr("Below is a magnificent canyon with deep gorges, high pinnacles and\n\
waterfalls plummeting hundreds of feet into mist. Everything in sight\n\
is carpeted with a tropical green.* The ocean is +.**\n") },
	{ strstr("You are flying over the ocean."),
	        { 74, 78, 78, 78, 68, 1, 0, 1 },
strstr("You bank over the water and your wingtips dip low to the green waves.  The\n\
sea is very shallow here and the white coral beds beneath us teem with \n\
colorful fish.****\n") },
	{ strstr("You are flying over the beach."),
	        { 71, 72, 79, 74, 68, 1, 80, 1 },
strstr("A warm gentle surf caresses the white coral beach here. The land rises\n\
sharply +.* The beach is lost in low cliffs and rocks +.*\n") },
	{ strstr("You are flying over a large lagoon."),
	        { 81, 72, 73, 82, 68, 1, 0, 1 },
strstr("Encircled by a coral reef, the palms and ferns in this sheltered spot\n\
have grown down to the water's very brink which winds tortuously inland.\n\
There looks like a small village +.***\n") },
	{ strstr("You are flying over a gently sloping plane."),
	        { 83, 71, 84, 85, 68, 1, 0, 1 },
strstr("This is where several alluvial fans and ancient lava flows have run\n\
together forming a fertile plane choked with vegetation. It would be\n\
impossible to land safely here.* The terrain is more rugged +.**\n") },
	{ strstr("You are flying through a gorge."),
	        { 0, 0, 86, 71, 68, 1, 102, 1 },
strstr("This narrow, steep sided canyon is lined with waving ferns. The floor is of\n\
light gravel with many freshets pouring from the walls and running along it.\n\
The gorge leads to the sea** +, and to a tumultuous origin +.\n") },
	{ strstr("You are flying over a plantation."),
	        { 85, 81, 71, 88, 68, 1, 89, 1 },
strstr("Rows of palms, papayas, mangoes, kiwi, as well as smaller fields of sugar\n\
cane and pineapple are growing here. It might be possible to land here, but\n\
I wouldn't advise it.* There looks like two small settlements +     \n\
and *+.\n") },
	{ strstr("You are over the ocean."),
	        { 72, 78, 79, 74, 68, 1, 0, 1 },
strstr("The deep green swells foam and roll into the shore **+*.\n") },
	{ strstr("You are flying along the coast."),
	        { 86, 72, 90, 73, 68, 1, 91, 1 },
strstr("The coastline here is very rocky with little or no sand. The surf in some\n\
places is violent and explodes in a shower of sparkling spray.\n\
There is a winding road below which closely follows the shore. ****\n") },
	{ strstr("This is a beautiful coral beach."),
	        { 106, 0, 107, 108, 73, 0, 0, 0 },
strstr("Fine silver sand kissed lightly by warm tropical waters stretches at least\n\
30 meters here from the ocean to under gently swaying palms +.***\n") },
	{ strstr("You are flying over a small fishing village."),
	        { 77, 74, 71, 82, 68, 1, 92, 1 },
strstr("A few thatched huts a short distance from the water and row of more modern\n\
bungalows on either side of a dirt road are all that is here. The road\n\
continues on ***+.\n") },
	{ strstr("You are flying over a clearing."),
	        { 88, 72, 74, 87, 68, 1, 93, 1 },
strstr("There is a dock here (big enough for a seaplane) leading to a grassy\n\
meadow and a road. Some people are having a party down there.  Below is\n\
a good landing site. ****\n") },
	{ strstr("You are flying over the shore."),
	        { 94, 75, 95, 96, 68, 1, 0, 1 },
strstr("Rocky lava flows or coarse sandy beaches are all that is here except for\n\
sparse herbs and weeds.****\n") },
	{ strstr("You are flying in a wide valley."),
	        { 95, 97, 86, 75, 68, 1, 98, 1 },
strstr("This is a shallow valley yet the floor is obscured by a thick mist.\n\
The valley opens to the sea +. The mist grows thicker +.**\n") },
	{ strstr("You are flying near the shore."),
	        { 96, 77, 75, 99, 68, 1, 0, 1 },
strstr("Very tall palm trees growing in neatly planted rows march off from the \n\
water here towards the hills and down to the flat lands *+.*\n\
There is a nice beach +.\n") },
	{ strstr("You are flying around the very tip of the island."),
	        { 95, 79, 90, 84, 68, 1, 0, 1 },
strstr("There is no beach here for sheer cliffs rise several hundred feet\n\
to a tree covered summit. Far below, the blue sea gnaws voraciously at\n\
the roots of these cliffs. The island bends around +** and +.\n") },
	{ strstr("You are flying along the coastline."),
	        { 99, 82, 88, 100, 68, 1, 101, 1 },
strstr("There is a narrow strip of sand here lined with ferns and shrubs, but very\n\
few trees. The beach is barley wide enough to land on. The beach continues\n\
on -.* There are some buildings +.*\n") },
	{ strstr("You are flying over several cottages and buildings"),
	        { 99, 82, 77, 87, 68, 1, 103, 1 },
strstr("The grounds here are landscaped with palm trees, ferns, orchids, and beds of\n\
flowering plumeria and antheriums. Directly below is a small ornate white\n\
house with a belltower, a lush green lawn, and a spurting fountain.\n\
Small dirt roads go + and +.**\n") },
	{ strstr("You are in a field of sugar cane."),
	        { 109, 110, 111, 112, 77, 0, 0, 0 },
strstr("These strong, thick canes give little shelter but many cuts and scrapes.\n\
There are some large trees ***+.\n") },
	{ strstr("You are flying over the ocean."),
	        { 95, 78, 90, 86, 68, 1, 0, 1 },
strstr("The water is a placid turquoise and so clear that fish and sharks\n\
many fathoms below are clearly visible.****\n") },
	{ strstr("You are on the coast road."),
	        { 113, 114, 115, 116, 79, 0, 0, 0 },
strstr("The road winds close to the shore here and the sound of crashing surf is\n\
deafening.* The water is +. The road continues - and -.\n") },
	{ strstr("You are on the main street of the village."),
	        { 117, 118, 119, 120, 81, 0, 0, 0 },
strstr("Thatched roofs and outrigger canoes, palm trees and vacation bungalows, and\n\
comely natives in a tropical paradise all make this a fantasy come true.\n\
There is an open bungalow +.*  The road continues - and -.\n") },
	{ strstr("You are at the sea plane dock."),
	        { 121, 122, 123, 124, 82, 0, 0, 0 },
strstr("Native girls with skin of gold, clad only in fragrant leis and lavalavas,\n\
line the dockside to greet you. A couple of ukulele plucking islanders and a\n\
keyboard player are adding appropriate music. A road crosses the clearing \n\
+*.  There are some tables set up +.*\n") },
	{ strstr("You are flying over the ocean."),
	        { 94, 83, 95, 96, 68, 1, 0, 1 },
strstr("Sea weeds and kelp surge in the waves off shore here.  The ocean becomes \n\
much deeper +.***\n") },
	{ strstr("You are flying along the coast."),
	        { 94, 84, 86, 83, 68, 1, 0, 1 },
strstr("The land is very low here with a river running into the sea +. There\n\
is a wide valley opening up +. The very tip of the island is +.*\n") },
	{ strstr("You are flying along the coast."),
	        { 94, 85, 83, 99, 68, 1, 0, 1 },
strstr("There are some secluded sandy stretches of beach here, but too many rocky\n\
outcroppings of lava to land. There is a nicer beach ***+.\n") },
	{ strstr("You are lost in a sea of fog."),
	        { 97, 104, 97, 97, 97, 1, 0, 1 },
strstr("What have you gotten us into?\n\
I cant see a thing! ****\n") },
	{ strstr("You are on a gravel wash."),
	        { 125, 126, 127, 128, 84, 0, 0, 0 },
strstr("The sound of cascading water is the background for a diluted chorus of \n\
gurgling, splashing, and enchantingly delicate singing. Great billows\n\
of steam are rising *+.**\n") },
	{ strstr("You are flying over a wide beach."),
	        { 96, 88, 85, 87, 68, 1, 105, 1 },
strstr("Unlike the leeward beaches, few coconut palms grow here but a well groomed\n\
lawn and garden with traipsing stone walks leads down to the sand.*\n\
There are some buildings +. Some trees are growing +.*\n") },
	{ strstr("You are flying over the ocean."),
	        { 100, 100, 87, 100, 68, 1, 0, 1 },
strstr("The sea is a perfectly clear blue with a white sandy bottom.  No coral\n\
grows underwater here, but the force of the waves is broken by the steep\n\
incline.****\n") },
	{ strstr("You are on a narrow strip of sand."),
	        { 129, 130, 131, 0, 87, 0, 0, 0 },
strstr("Rather coarse sand makes this beach very steep and only a few meters wide.\n\
A fresh ocean breeze is rustling the ferns **+.*\n") },
	{ strstr("This is Fern Canyon."),
	        { 0, 0, 132, 133, 76, 0, 0, 0 },
strstr("Delicate waving ferns flourish here, suckled by warm water dripping from \n\
every fissure and crevice in the solid rock walls.\n\
The canyon winds **-, and -.\n") },
	{ strstr("This is the front lawn."),
	        { 134, 135, 136, 137, 88, 0, 0, 0 },
strstr("There is a small fountain here where the driveway meets the lawn.\n\
Across the driveway, +, is an ornate white house with and elegant \n\
woodworking.  The bargeboards are carved with fylfots, the ancient \n\
symbols of luck.  Even a bell tower has been built here.*  There is a \n\
road + which turns into the driveway.*\n") },
	{ strstr("You have just crossed the crest of a mountain."),
	        { 97, 79, 86, 71, 68, 1, 0, 1 },
strstr("The fog vanished mysteriously as we flew over the crest.*\n\
Far + I can see the ocean.**\n") },
	{ strstr("You are on a sandy beach."),
	        { 138, 139, 140, 0, 99, 0, 0, 0 },
strstr("This is the only good beach on the weather side of the island. Fine coral\n\
sand, a fresh sea breeze, and dramatic surf add to its appeal.**\n\
Stone steps lead to the gardens +.*\n") },
	{ strstr("You are among palm trees near the shore."),
	        { 141, 80, 142, 143, 73, 0, 0, 0 },
strstr("Arching coconut palms laden with fruit provide a canopy for the glistening\n\
white sand and sparse grasses growing here. The forest grows denser +.\n\
The ocean is +.**\n") },
	{ strstr("You are walking along the beach."),
	        { 144, 0, 145, 80, 73, 0, 0, 0 },
strstr("The warm tropical waters nuzzle your ankles as you walk. Above is a fiercely\n\
blue sky. The slope of the sand is so gentle that two hundred meters\n\
offshore the water is only knee deep.** There are some rocks +.*\n") },
	{ strstr("You are walking along the beach."),
	        { 146, 0, 80, 147, 73, 0, 0, 0 },
strstr("Many beautiful shells have been washed up here including bright yellow \n\
cowries, chocolate colored murex, orange conchs, striped tritons and the\n\
deadly cone shells.****\n") },
	{ strstr("You are in a papaya grove."),
	        { 148, 89, 149, 150, 77, 0, 0, 0 },
strstr("Green slender trees no taller than three meters bulge with their\n\
orange succulent fruit. There are some tall trees +.***\n") },
	{ strstr("You are in a field of pineapple."),
	        { 89, 151, 152, 153, 77, 0, 0, 0 },
strstr("The sharp dagger like pineapple leaves can pierce the flesh and hold fast\n\
a skewered victim with tiny barbs.* The field ends +.**\n") },
	{ strstr("You are in a field of kiwi plants."),
	        { 149, 154, 155, 89, 77, 0, 0, 0 },
strstr("Round hairy fruit hang from staked vines here. There are some trees +\n\
and +. The field ends in a dirt road +.*\n") },
	{ strstr("You are in a large grove of coconuts."),
	        { 150, 153, 89, 156, 77, 0, 0, 0 },
strstr("These trees are much taller than any growing near the shore plus the fat,\n\
juicy coconuts have been selectively cultivated. The grove continues\n\
+, +, *and +.\n") },
	{ strstr("You are in the woods."),
	        { 157, 91, 158, 116, 79, 0, 0, 0 },
strstr("Tropical undergrowth makes the going rough here. Sword ferns give no strong\n\
foot hold and the dangling vines would gladly throttle one. Strange cackling\n\
noises are coming from somewhere +.***\n") },
	{ strstr("You are at the shore."),
	        { 91, 0, 159, 145, 79, 0, 0, 0 },
strstr("Explosions of surf jetting out of underwater tunnels here make it\n\
impossible to climb down to a small cave entrance below.  Only at rare\n\
minus tides would it be possible to enter.***  The beach is better +.\n") },
	{ strstr("You are on the coast road."),
	        { 158, 161, 162, 91, 79, 0, 0, 0 },
strstr("The road is beginning to turn inland.* I can here the surf +. The road\n\
continues +.*\n") },
	{ strstr("The road winds deeper into the trees."),
	        { 163, 142, 91, 164, 79, 0, 0, 0 },
strstr("Only narrow sunbeams filter through the foliage above. The moist rich earth\n\
has nurtured a myriad of trees, shrubs, and flowers to grow here. The\n\
road continues - and *- from here.*\n") },
	{ strstr("This is the front porch of the bungalow."),
	        { 165, 92, 0, 0, 81, 0, 0, 0 },
strstr("These wooden steps and porch are very bucolic. A little woven mat on the \n\
doorstep reads \"Don't Tread on Me\". The open front door is +.\n\
A stone walk leads to the main street +.**\n") },
	{ strstr("You are on a path leading to the lagoon."),
	        { 92, 166, 167, 168, 81, 0, 0, 0 },
strstr("This path trampled fern, grass, sapling, and anything else that got in its\n\
way.* The water is +.**\n") },
	{ strstr("This is a dirt road."),
	        { 169, 118, 170, 92, 81, 0, 0, 0 },
strstr("**The road continues on - here for some distance. A village is +.\n") },
	{ strstr("You are on a dirt road."),
	        { 171, 118, 92, 172, 81, 0, 0, 0 },
strstr("**There is a small village +. The road continues +.\n") },
	{ strstr("You are on a dirt road."),
	        { 173, 93, 174, 175, 82, 0, 0, 0 },
strstr("The light tan soil of the road contrasts artistically with the lush green\n\
vegetation and seering blue sky.*  There is a clearing and many people +.\n\
The road continues - and -.\n") },
	{ strstr("You are at the seaplane dock."),
	        { 93, 0, 176, 177, 82, 0, 0, 0 },
strstr("Several muscular, bronze skinned men greet you warmly as you pass under\n\
a thatched shelter above the dock here. Polynesian hospitality.\n\
There is a clearing +.* A trail runs around the lagoon + and +.\n") },
	{ strstr("There are some tables on the lawn here."),
	        { 121, 122, 123, 93, 82, 0, 0, 0 },
strstr("Hors d'oeuvres, canapes, mixed drinks, and various narcotic drugs along with\n\
cartons of Di Gel fill the tables to overflowing. Several other guests are\n\
conversing and talking excitedly****.\n") },
	{ strstr("You are nosing around in the bushes."),
	        { 124, 124, 93, 124, 82, 0, 0, 0 },
strstr("There is little here but some old beer cans. You are making fools out of\n\
us in front of the other guests.** It would be best to go -.*\n") },
	{ strstr("You are walking in a dry stream bed."),
	        { 178, 98, 179, 0, 84, 0, 0, 0 },
strstr("The large cobblestones are difficult to walk on. No sunlight reaches\n\
below a white canopy of fog seemingly generated from *+.  A dirt path \n\
along the wash is +. A high bank is impossible to climb +.\n") },
	{ strstr("You are at the thermal pools."),
	        { 98, 0, 180, 181, 84, 0, 0, 0 },
strstr("Several steaming fumaroles and spluttering geysers drenched by icy mountain\n\
waters from a nearby waterfall heat half a dozen natural pools to a\n\
delicious 42 degrees. Enchantingly beautiful singing seems to flow from the\n\
water itself as it tumbles down the falls.*** There is a mossy entrance\n\
to a cave +.\n") },
	{ strstr("You are in the woods."),
	        { 127, 180, 182, 98, 84, 0, 0, 0 },
strstr("Coniferous trees girded by wild huckleberries, elderberries, salmonberries\n\
and thimbleberries enjoy a less tropical climate here in the high mountains.\n\
*The sound of rushing water is coming from +.**\n") },
	{ strstr("You are on a dirt trail."),
	        { 179, 181, 98, 0, 84, 0, 0, 0 },
strstr("The trail seems to start here and head -.** High cliffs border the \n\
trail +.\n") },
	{ strstr("You are  walking along the beach."),
	        { 183, 101, 184, 0, 87, 0, 0, 0 },
strstr("A rather unnerving surf explodes onto the beach here and dashes itself into\n\
spray on the steep incline. The beach continues + and +.**\n") },
	{ strstr("You are walking along the beach."),
	        { 101, 185, 186, 0, 87, 0, 0, 0 },
strstr("This is not a very nice beach. The coarse sand hurts my feet.****\n") },
	{ strstr("You are walking through some ferns."),
	        { 184, 186, 187, 101, 87, 0, 0, 0 },
strstr("This is a wide field growing only ferns and small shrubs.** The \n\
ocean is *+.\n") },
	{ strstr("You are in a narrow canyon."),
	        { 0, 0, 188, 102, 76, 0, 0, 0 },
strstr("The steep sides here squeeze a little freshet through a gauntlet like\n\
series of riffles and pools.****\n") },
	{ strstr("The canyon is much wider here."),
	        { 0, 0, 102, 189, 76, 0, 0, 0 },
strstr("The sheer rock walls rise 10 meters to the forest above. A slender \n\
waterfall careens away from the face of the rock high above and showers\n\
the gravel floor with sparkling raindrops.** The canyon continues -\n\
and -.\n") },
	{ strstr("You are on the front porch of the cottage."),
	        { 190, 103, 0, 0, 0, 0, 0, 0 },
strstr("Several giggling native girls came running down the steps as you approached\n\
and headed on down the road.  On the fern rimmed porch is a small table with\n\
matching white wrought iron chairs cushioned with red velvet.  The front\n\
door leads -.  The lawn and fountain are +.**\n") },
	{ strstr("You are in a palm grove."),
	        { 103, 191, 192, 105, 88, 0, 0, 0 },
strstr("****\n") },
	{ strstr("You are on a dirt road."),
	        { 193, 192, 245, 103, 88, 0, 0, 0 },
strstr("There is a large village +. The road cleaves a coconut plantation +.\n\
A small dirt road goes -, and a drive way peals off +.\n") },
	{ strstr("You are in a field of small shrubs."),
	        { 184, 186, 103, 187, 88, 0, 0, 0 },
strstr("**Pine and other coniferous saplings have been planted here.  The rich brown\n\
soil is well tilled and watered.  Across a large lawn, there is a small\n\
cottage +. I can feel a delicious sea breeze blowing from +.\n") },
	{ strstr("The beach is pretty rocky here."),
	        { 194, 105, 195, 0, 96, 0, 0, 0 },
strstr("Dangerous surf and lava outcroppings make this a treacherous strand.\n\
The beach is nicer* +.**\n") },
	{ strstr("The beach is almost 10 meters wide here."),
	        { 105, 183, 196, 0, 99, 0, 0, 0 },
strstr("The sand has become more coarse and the beach steeper.* It gets \n\
worse +.**\n") },
	{ strstr("You are in the gardens."),
	        { 195, 196, 197, 105, 99, 0, 0, 0 },
strstr("Lush green lawns studded with palms and benches stretch as far as the eye\n\
can see.** A path leads -. Stone steps lead down to the beach +.\n") },
	{ strstr("You are on the coast road."),
	        { 198, 106, 163, 199, 73, 0, 0, 0 },
strstr("The forest is dense on either side and conceals the road from anyone\n\
approaching it.**  The road continues - and -.\n") },
	{ strstr("You are in the forest."),
	        { 116, 107, 91, 106, 73, 0, 0, 0 },
strstr("There are trees and ferns all around.****\n") },
	{ strstr("You are in the forest."),
	        { 199, 108, 106, 146, 73, 0, 0, 0 },
strstr("There are trees and ferns all around.****\n") },
	{ strstr("You are in a copse."),
	        { 142, 107, 145, 80, 0, 0, 0, 0 },
strstr("This is a secret hidden thicket only noticeable from the beach. Someone\n\
has been digging here recently.****\n") },
	{ strstr("You are at the tide pools."),
	        { 91, 0, 114, 107, 79, 0, 0, 0 },
strstr("These rocks and pools are the home for many sea anemones and crustaceans.\n\
**The surf is very rough +. There is a nice beach +.\n") },
	{ strstr("You are in the forest."),
	        { 199, 108, 143, 0, 73, 0, 0, 0 },
strstr("This is a shallow depression sheltered from the wind by a thick growth of \n\
thorny shrubs. It looks like someone has camped here. There is a fire pit\n\
with some dry sticks and grass nearby.* The beach is +.* The thorny\n\
shrubs block the way -.\n") },
	{ strstr("You are at the mouth of the lagoon."),
	        { 200, 0, 108, 201, 74, 0, 0, 0 },
strstr("The beach ends here where the coral reef rises to form a wide lagoon\n\
bending inland. A path winds around the lagoon to the -.*\n\
The beach continues on -. Only water lies +.\n") },
	{ strstr("You are in a breadfruit grove."),
	        { 202, 109, 203, 204, 77, 0, 0, 0 },
strstr("The tall trees bend leisurely in the breeze, holding many round breadfruits\n\
close to their large serrated leaves.  There are coconut palms +,\n\
*+, and +.\n") },
	{ strstr("You are in a grove of mango trees."),
	        { 203, 111, 205, 109, 77, 0, 0, 0 },
strstr("The juicy yellow red fruits are nearly ripe on the trees here. There are\n\
some coconut palms +. There are some vines +. There is a road +.*\n") },
	{ strstr("You are in a grove of coconut palms."),
	        { 204, 112, 109, 206, 77, 0, 0, 0 },
strstr("All I can see around us are palm trees.****\n") },
	{ strstr("You are in a coconut grove."),
	        { 110, 207, 208, 209, 77, 0, 0, 0 },
strstr("There are countless trees here.****\n") },
	{ strstr("You are in a field of pineapple."),
	        { 154, 208, 210, 110, 77, 0, 0, 0 },
strstr("The sharp leaves are cutting me to ribbons. There is a road **+.\n\
More pineapple +.\n") },
	{ strstr("You are in a coconut grove."),
	        { 112, 209, 110, 211, 77, 0, 0, 0 },
strstr("There is a field of pineapple **+.*\n") },
	{ strstr("You are on the edge of a kiwi and pineapple field."),
	        { 111, 152, 155, 110, 77, 0, 0, 0 },
strstr("An irrigation ditch separates the two fields here. There is a road **+.*\n") },
	{ strstr("This is a dirt road."),
	        { 205, 210, 212, 111, 77, 0, 0, 0 },
strstr("The road runs - and - here.**\n") },
	{ strstr("You are in a palm grove."),
	        { 206, 211, 112, 213, 77, 0, 0, 0 },
strstr("There are palm trees all around us.****\n") },
	{ strstr("You are on the edge of a small clearing."),
	        { 157, 113, 157, 157, 79, 0, 0, 0 },
strstr("The ground is rather marshy here and darting in and out of the many tussocks\n\
is a flock of wild chicken like fowl.****\n") },
	{ strstr("You are in the woods."),
	        { 158, 115, 215, 113, 79, 0, 0, 0 },
strstr("You have walked a long way and found only trees. ****\n") },
	{ strstr("You are walking along the shore."),
	        { 115, 0, 214, 114, 86, 0, 0, 0 },
strstr("You are now about 10 meters above the surf on a gently rising cliffside.**\n\
The land rises +. There is a beach far +.\n") },
	{ strstr("You are just inside the entrance to the sea cave."),
	        { 246, 114, 0, 0, 114, 1, 0, 0 },
strstr("The sound of water dripping in darkness and the roar of the ocean just outside\n\
create a very unwelcoming atmosphere inside this cave. Only on rare occasions\n\
such as this is it possible to enter the forbidden catacombs... The cave\n\
continues -.***\n") },
	{ strstr("You are in a secret nook beside the road."),
	        { 115, 159, 162, 91, 79, 0, 0, 0 },
strstr("Hidden from all but the most stalwart snoopers are some old clothes, empty\n\
beer cans and a trash baggie full of used Huggies and ordure. Lets get\n\
back to the road +.***\n") },
	{ strstr("You are on the coast road."),
	        { 215, 214, 0, 115, 86, 0, 0, 0 },
strstr("The road turns abruptly - here, avoiding the cliffs near the shore\n\
+ and +.*\n") },
	{ strstr("You are on a dirt road."),
	        { 216, 116, 113, 141, 79, 0, 0, 0 },
strstr("The roadside is choked with broad leaved plants fighting for every breath of\n\
sunshine. The palm trees are taller than at the shore yet bend over the road \n\
forming a canopy. The road continues *- and *-.\n") },
	{ strstr("You have discovered a hidden thicket near the road."),
	        { 163, 142, 116, 106, 73, 0, 0, 0 },
strstr("Stuffed into a little bundle here is a bloody silken robe and many beer cans.\n\
*Some droplets of blood and a major spill sparkle farther +.\n\
The road is +.*\n") },
	{ strstr("You are in the living room."),
	        { 0, 117, 217, 218, 0, 0, 0, 0 },
strstr("A decorative entry with fresh flowers and wall to wall carpeting leads into\n\
the living room here where a couch and two chairs converse with an end table.\n\
*The exit is +.* The bedroom is +.\n") },
	{ strstr("You are at the lagoon."),
	        { 118, 0, 167, 168, 81, 0, 0, 0 },
strstr("There are several outrigger canoes pulled up on a small beach here and a\n\
catch of colorful fish is drying in the sun. There are paths leading \n\
off -*, -, and -.\n") },
	{ strstr("You are at the lagoon."),
	        { 118, 0, 170, 166, 81, 0, 0, 0 },
strstr("This is a grassy little spot near the water. A sightly native girl is frolicing\n\
in the water close to shore here.** The path continues - and -. \n") },
	{ strstr("You are at the lagoon."),
	        { 118, 0, 166, 172, 81, 0, 0, 0 },
strstr("The path meanders through tussocks of grass, ferns, and thorny bushes here\n\
and continues on **- and -.\n") },
	{ strstr("You are in the woods."),
	        { 219, 119, 220, 92, 81, 0, 0, 0 },
strstr("There are plenty of ferns and thorny bushes here! ****\n") },
	{ strstr("You are on a dirt road."),
	        { 220, 167, 199, 119, 74, 0, 0, 0 },
strstr("The road winds rather close to a large lagoon here and many sedges and tall\n\
grasses line the shoulder *+. The road continues - and -.\n") },
	{ strstr("You are in the woods beside the road."),
	        { 221, 120, 92, 222, 81, 0, 0, 0 },
strstr("The forest grows darker +. The road is +.**\n") },
	{ strstr("The road crosses the lagoon here."),
	        { 222, 0, 120, 174, 81, 0, 0, 0 },
strstr("Coursing through the trees, the road at this point bridges a watery finger\n\
of the lagoon.* The water is +. The road continues - and -.\n") },
	{ strstr("You are in a coconut palm grove."),
	        { 223, 121, 224, 225, 82, 0, 0, 0 },
strstr("The tall palms are planted about 30 feet apart with a hardy deep green grass\n\
filling the spaces in between.  There are tire tracks through the grass. The\n\
grove continues -. There is a road +.**\n") },
	{ strstr("You are walking along a dirt road."),
	        { 224, 176, 172, 121, 82, 0, 0, 0 },
strstr("You are nearing the lagoon.** The road continues - and -.\n") },
	{ strstr("You are on a dirt road."),
	        { 225, 177, 121, 226, 82, 0, 0, 0 },
strstr("The road turns abruptly - here, entering a grove of palm trees.* The road\n\
also continues - toward the lagoon.*\n") },
	{ strstr("You are on a trail running around the lagoon."),
	        { 172, 0, 0, 122, 82, 0, 0, 0 },
strstr("The dark waters brush the trail here and the path crosses a bridge +.\n\
There is deep water + and +. The trail continues -.\n") },
	{ strstr("This is the mouth of the lagoon."),
	        { 175, 0, 122, 227, 82, 0, 0, 0 },
strstr("The coral reef wraps around a natural bay here to create a wide lagoon which\n\
winds tortuously inland.** A trail goes around the lagoon +. The beach\n\
is +.\n") },
	{ strstr("You are in a dry stream bed."),
	        { 0, 125, 0, 0, 84, 0, 0, 0 },
strstr("The dry wash drains over a tall precipice here into a turbid morass below. The\n\
most noisome stench imaginable is wafting up to defile our nostrils. Above,\n\
the lurid sun glows brown through a strange mist.* The only direction \n\
I'm going is -.**\n") },
	{ strstr("You are on a dirt path along the wash."),
	        { 0, 128, 125, 228, 84, 0, 0, 0 },
strstr("This path looks more like a deer trail. It scampers away ***+.\n") },
	{ strstr("The thermal pools flow into a stream here."),
	        { 127, 0, 229, 126, 84, 0, 0, 0 },
strstr("The gurgling hot waters pour over boulders into a swiftly flowing\n\
stream **+. The pools are +.\n") },
	{ strstr("You are at the entrance to a cave."),
	        { 128, 230, 126, 0, 84, 0, 0, 0 },
strstr("A tall narrow fissure in the rock cliffs here has become a well traveled\n\
passage way. A hoof beaten dirt path leads directly into it. A curl of\n\
steam is trailing from a corner of the fissure's gaping mouth. The path\n\
leads - and -. The pools are +.*\n") },
	{ strstr("You are in the woods."),
	        { 182, 229, 182, 127, 84, 0, 0, 0 },
strstr("Wild berry bushes plump with fruit and thorns tangle your every effort to\n\
proceed.* The sound of rushing water is +.**\n") },
	{ strstr("You are walking along the beach."),
	        { 139, 129, 184, 0, 99, 0, 0, 0 },
strstr("Some dunes here progress inland and make it impossible to get very far in that\n\
direction. The beach continues - and -.* The ocean is +.\n") },
	{ strstr("You are in the dunes."),
	        { 183, 101, 184, 129, 87, 0, 0, 0 },
strstr("The endless rolling and pitching sand dunes are enough to make one very queasy!\n\
The only way I'm going is ***+.\n") },
	{ strstr("This is a lousy beach."),
	        { 130, 0, 0, 0, 87, 0, 0, 0 },
strstr("Volcanic and viciously sharp bitted grains of sand here bite like cold steel\n\
into my tender feet. I refuse to continue on. Let's get out of here. The\n\
beach is better +.***\n") },
	{ strstr("You are in a field of sparse ferns."),
	        { 131, 185, 187, 130, 87, 0, 0, 0 },
strstr("The lava rock outcroppings here will support few plants. There is more \n\
vegetation +. There is a nice beach +.* The ocean is +.\n") },
	{ strstr("You are in the woods."),
	        { 131, 131, 137, 131, 87, 0, 0, 0 },
strstr("Young trees and tall shrubs grow densely together at this distance from the \n\
shore.** The trees grow thicker +.*\n") },
	{ strstr("The canyon is no wider than a foot here."),
	        { 0, 0, 0, 132, 0, 0, 0, 0 },
strstr("The freshet is gushing through the narrow trough, but the canyon has grown\n\
too narrow to follow it any farther.*** I guess we'll have to go -.\n") },
	{ strstr("You are in a narrow part of the canyon."),
	        { 0, 0, 133, 232, 76, 0, 0, 0 },
strstr("The two sheer sides are no more than a few meters apart here. There is a stone\n\
door in the wall +. The gravelly floor runs with tiny rivulets seeping \n\
from the ground itself.* The canyon continues - and -.\n") },
	{ strstr("You are in the drawing room."),
	        { 0, 134, 0, 0, 0, 0, 0, 0 },
strstr("Exquisitely decorated with plants and antique furniture of superb\n\
craftsmanship, the parlor reflects its owners impeccable taste.  The tropical\n\
sun is streaming in through open shutters *+.  There doesn't seem \n\
to be anybody around.  A large immaculate oaken desk is visible in the\n\
study and it even has a old fashioned telephone to complete the decor.**\n") },
	{ strstr("You are in a palm grove."),
	        { 135, 191, 233, 191, 88, 0, 0, 0 },
strstr("Grassy rows of palms stretch as far as I can see.** There is a road +.*\n") },
	{ strstr("You are on a dirt road."),
	        { 136, 233, 234, 135, 88, 0, 0, 0 },
strstr("The road winds through a coconut palm grove here. It continues on - \n\
and -.**\n") },
	{ strstr("The road leads to several large buildings here."),
	        { 235, 136, 236, 237, 88, 0, 0, 0 },
strstr("There is a clubhouse +,* a large barn and stable +, and a garage of \n\
similar construct to the barn +.\n") },
	{ strstr("This part of the beach is impassable."),
	        { 0, 138, 0, 0, 96, 0, 0, 0 },
strstr("The huge rocks and thunderous surf here would pound our frail bodies to pulp\n\
in an instant.* The only direction I'm going is -.**\n") },
	{ strstr("You are in the gardens."),
	        { 195, 140, 197, 138, 96, 0, 0, 0 },
strstr("So much green grass is a pleasure to the eyes.****\n") },
	{ strstr("You are in the gardens."),
	        { 140, 183, 197, 139, 99, 0, 0, 0 },
strstr("Beautiful flowers and shrubs surround a little goldfish pond.****\n") },
	{ strstr("You are on a stone walk in the garden."),
	        { 195, 196, 238, 140, 99, 0, 0, 0 },
strstr("The walk leads to a road **+.*\n") },
	{ strstr("You are in the forest near the road."),
	        { 198, 141, 216, 198, 73, 0, 0, 0 },
strstr("There are many thorny bushes here!****\n") },
	{ strstr("You are at a fork in the road."),
	        { 239, 146, 141, 170, 73, 0, 0, 0 },
strstr("Two roads come together in the forest here. One runs -,* the other \n\
runs - and -.\n") },
	{ strstr("You are on a dirt path around the lagoon."),
	        { 170, 147, 146, 0, 74, 0, 0, 0 },
strstr("The still waters reflect bending palms and a cloudless sky. It looks like\n\
the path runs into a clearing +. The path continues -.**\n") },
	{ strstr("You are drowning in the lagoon."),
	        { 201, 201, 147, 201, 74, 0, 0, 0 },
strstr("I suggest you get out before you become waterlogged.****\n") },
	{ strstr("You are in a coconut palm grove."),
	        { 202, 148, 203, 204, 77, 0, 0, 0 },
strstr("****\n") },
	{ strstr("You are in a palm grove."),
	        { 202, 149, 205, 148, 77, 0, 0, 0 },
strstr("****\n") },
	{ strstr("You are in a palm grove."),
	        { 202, 150, 148, 206, 77, 0, 0, 0 },
strstr("****\n") },
	{ strstr("You are on a dirt road."),
	        { 203, 155, 212, 149, 77, 0, 0, 0 },
strstr("*This road ends here at a palm grove but continues on - for quite\n\
some way.**\n") },
	{ strstr("You are in a coconut palm grove."),
	        { 204, 156, 150, 213, 77, 0, 0, 0 },
strstr("****\n") },
	{ strstr("You are in a coconut grove."),
	        { 151, 219, 208, 209, 77, 0, 0, 0 },
strstr("*The grove ends +.**\n") },
	{ strstr("You are in a coconut grove."),
	        { 152, 207, 239, 151, 77, 0, 0, 0 },
strstr("**There is a dirt road +.*\n") },
	{ strstr("You are in a coconut grove."),
	        { 153, 207, 151, 211, 77, 0, 0, 0 },
strstr("****\n") },
	{ strstr("This is a dirt road."),
	        { 205, 239, 212, 154, 77, 0, 0, 0 },
strstr("The road continues - and -.**\n") },
	{ strstr("You are in a coconut grove."),
	        { 153, 209, 153, 213, 77, 0, 0, 0 },
strstr("****\n") },
	{ strstr("You are in the woods near the road."),
	        { 205, 210, 212, 155, 77, 0, 0, 0 },
strstr("There are many thorny bushes here!****\n") },
	{ strstr("You are in a coconut grove."),
	        { 213, 213, 156, 234, 88, 0, 0, 0 },
strstr("***The grove ends in a clearing +.\n") },
	{ strstr("You are walking along some high cliffs."),
	        { 162, 0, 0, 159, 86, 0, 0, 0 },
strstr("The island bends sharply + here with high cliffs -\n\
and -. The cliffs are lower +.\n") },
	{ strstr("You are at the coast road turn around."),
	        { 0, 162, 0, 158, 90, 0, 0, 0 },
strstr("The coast road ends here in a lookout with a view of 100 kilometers of blue\n\
sea and 100 meters of rugged cliff. Far below the waves crash against rocks.\n\
****\n") },
	{ strstr("You are in the woods near the road."),
	        { 216, 163, 216, 198, 79, 0, 257, 0 },
strstr("These thorny bushes are killing me.****\n") },
	{ strstr("You are in the kitchen."),
	        { 0, 0, 0, 165, 0, 0, 0, 0 },
strstr("A small gas stove and a refrigerator are all the only appliances here. The\n\
gas oven has been left on and the whole room is reeking with natural gas.\n\
One spark from a match and.... The door out is ***+.\n") },
	{ strstr("You are in the bedroom."),
	        { 0, 0, 165, 0, 0, 0, 0, 0 },
strstr("A soft feather comforter on top of layers of Answer blankets make this a very\n\
luxurious place to sleep indeed. There are also some end tables and a dresser\n\
here.** The living room is +.*\n") },
	{ strstr("You are in the woods."),
	        { 207, 169, 220, 221, 81, 0, 0, 0 },
strstr("There seems to be a clearing +.***\n") },
	{ strstr("You are in the woods near the road."),
	        { 219, 170, 239, 169, 81, 0, 0, 0 },
strstr("*As far as I can tell, there are two roads + and +.*\n") },
	{ strstr("You are in the woods."),
	        { 207, 171, 219, 222, 81, 0, 0, 0 },
strstr("The forest is clearer +.***\n") },
	{ strstr("You are on the lagoon's inland finger."),
	        { 0, 172, 171, 172, 81, 0, 0, 0 },
strstr("It is impossible to follow the lagoon any farther inland because of sharp\n\
and very painful sedges.* The road is +.**\n") },
	{ strstr("You are in a grassy coconut grove."),
	        { 240, 173, 224, 241, 82, 0, 0, 0 },
strstr("The tall palms provide a perfect canopy for the lush green grass.***\n\
There is a road +.\n") },
	{ strstr("You are near the lagoon's inland finger."),
	        { 0, 174, 0, 173, 82, 0, 0, 0 },
strstr("Very sharp sedges make it impossible to follow the lagoon any farther inland.\n\
*There is a road +.**\n") },
	{ strstr("You are on a dirt road."),
	        { 241, 175, 173, 226, 82, 0, 0, 0 },
strstr("The road winds through a coconut grove here and continues - and -.**\n") },
	{ strstr("You are in the woods near the road."),
	        { 226, 226, 175, 226, 82, 0, 0, 0 },
strstr("**The road is +.*\n") },
	{ strstr("This is a beach?"),
	        { 227, 227, 177, 0, 82, 0, 0, 0 },
strstr("Hard jagged rocks that pierce with every footstep hardly comprise a beach.**\n\
Let's go -.*\n") },
	{ strstr("The trail is lost in the woods here."),
	        { 241, 241, 179, 241, 84, 0, 0, 0 },
strstr("I suppose the animals that use this trail all depart in different directions\n\
when they get this far into the woods.** The trail goes -.*\n") },
	{ strstr("You are on the bank of a stream."),
	        { 182, 0, 242, 180, 84, 0, 0, 0 },
strstr("The stream falls over several small boulders here and continues on **-.*\n") },
	{ strstr("You are just inside the cave."),
	        { 181, 267, 0, 0, 0, 0, 0, 0 },
strstr("A steamy hot breath is belching from the depths of the earth within.* The\n\
cave  continues -.**\n") },
	{ strstr("You are just inside the cave entrance."),
	        { 274, 0, 0, 0, 0, 0, 0, 0 },
strstr("The air is hot and sticky inside. The cave continues -. There is a \n\
stone door in the wall +.  A wooden sign in the dust reads in old elven\n\
runes, \"GSRF KDIRE NLVEMP!\".**\n") },
	{ strstr("You are at the edge of a huge chasm."),
	        { 0, 0, 189, 0, 76, 0, 0, 0 },
strstr("Several hundred feet down I can see the glimmer of placid water. The\n\
rivulets drain over the edge and trickle down into the depths. It is \n\
impossible to climb down without a rope.** The canyon continues -.*\n") },
	{ strstr("You are on a dirt road."),
	        { 192, 241, 240, 191, 88, 0, 0, 0 },
strstr("The road winds through a coconut grove here. The road continues on -\n\
and -.**\n") },
	{ strstr("You are in a coconut palm grove near the road."),
	        { 193, 233, 213, 192, 88, 0, 0, 0 },
strstr("***The road is +.\n") },
	{ strstr("You are at the clubhouse."),
	        { 0, 193, 0, 0, 0, 0, 0, 0 },
strstr("The clubhouse is built over the most inland part of the lagoon.  Tropical\n\
bananas and fragrant frangipani grow along the grassy shore.  Walking across\n\
the short wooden bridge, we enter.  Along one wall is a bar with only a few\n\
people seated at it.  The restaurant and dance floor are closed off with\n\
a 2 inch nylon rope. ****\n") },
	{ strstr("You are in the stables."),
	        { 0, 0, 0, 193, 0, 0, 0, 0 },
strstr("Neighing horses snacking on hay and oats fill the stalls on both sides of\n\
the barn.  It is rather warm in here but that is not the most offensive\n\
part.  The old boards of the barn part just enough to let in dust laden\n\
shafts of light.  Flies swarm overhead and strafe the ground for dung.\n\
My nose is beginning to itch. ****\n") },
	{ strstr("You are in the old garage."),
	        { 0, 0, 193, 0, 0, 0, 0, 0 },
strstr("This is an old wooden building of the same vintage as the stables.  Beneath\n\
a sagging roof stand gardening tools and greasy rags.  Parked in the center\n\
is an underpowered Plymouth Volare' with a red and white striped golf cart\n\
roof. ****\n") },
	{ strstr("You are on a dirt road."),
	        { 197, 197, 243, 197, 85, 0, 0, 0 },
strstr("The road leads to a beautiful formal garden laced with stone walks and tropical\n\
flowers and trees.** The road continues -. A walk leads -.\n") },
	{ strstr("You are on a dirt road."),
	        { 210, 199, 198, 220, 73, 0, 0, 0 },
strstr("The road runs - and -.**\n") },
	{ strstr("You are in a coconut grove near the road."),
	        { 234, 223, 234, 233, 88, 0, 0, 0 },
strstr("***The road is +.\n") },
	{ strstr("You are on a dirt road."),
	        { 233, 225, 223, 226, 82, 0, 0, 0 },
strstr("The road continues - and -.**\n") },
	{ strstr("The stream plummets over a cliff here."),
	        { 182, 0, 0, 229, 84, 0, 0, 0 },
strstr("Falling 10 agonizing meters into spray, only droplets of the stream are\n\
left to dance off the floor below.  I thought I saw a sparkle of gold\n\
at the bottom of the falls, but now it is gone.  There is no way down,\n\
even with a strong rope. ****\n") },
	{ strstr("You are on a dirt road."),
	        { 0, 0, 244, 238, 85, 0, 0, 0 },
strstr("**The road continues - and -.\n") },
	{ strstr("You are on a dirt road."),
	        { 0, 245, 0, 243, 88, 0, 0, 0 },
strstr("*The road continues -* and -.\n") },
	{ strstr("You are on a dirt road."),
	        { 244, 234, 213, 136, 88, 0, 0, 0 },
strstr("The road goes -* and *-.\n") },
	{ strstr("You are in a low passage."),
	        { 247, 160, 0, 0, 0, 0, 0, 0 },
strstr("The passage is partially flooded here and it may be hazardous to proceed.\n\
Water is surging from the tunnel and heading out to sea. Strange moaning\n\
noises rise above the rushing of the water.  They are as thin as a whispering\n\
wind yet penetrate to my very soul.  I think we have come too far...\n\
The passage continues -.***\n") },
	{ strstr("The walls are very close together here."),
	        { 248, 0, 0, 0, 0, 0, 0, 0 },
strstr("I can barely squeeze through the jagged opening. Slimy sea weeds provide\n\
no footing at all. This tunnel seems to be an ancient lava tube. There is\n\
a large room -.***\n") },
	{ strstr("You are in the cathedral room."),
	        { 249, 251, 249, 251, 0, 0, 0, 0 },
strstr("Your light casts ghostly shadows on the walls but cannot pierce the \n\
engulfing darkness overhead. The sound of water dripping echoes in the void.\n\
*I can see no passages leading out of this room.  We have definitely\n\
come too far.*** \n") },
	{ strstr("You are walking through a very round tunnel."),
	        { 252, 0, 0, 0, 252, 1, 0, 0 },
strstr("The round walls of this tunnel are amazingly smooth to the touch. A little\n\
trickle of water flows down the center. The tunnel climbs steadily +.\n\
The cave is beginning to flood again!  Let's get out of here! ***\n") },
	{ strstr("You are in the cathedral anteroom."),
	        { 0, 0, 0, 248, 253, 1, 0, 0 },
strstr("This small chamber with a flat stone floor is to one side of the cathedral \n\
room. We appear to be at the bottom of a tall narrow shaft. There are many \n\
puddles of water here. A staircase hewn from solid rock and black lava \n\
leads up.*** The cathedral room is +.\n") },
	{ strstr("You are in a wide chamber."),
	        { 0, 0, 248, 254, 0, 0, 0, 0 },
strstr("Water is sprinkling from the ceiling here. A shallow pool populated by a \n\
myriad of blind white creatures sparkles in your light. Tiny shrimp and\n\
crabs scurry away, frightened by the blinding rays.** The cave \n\
continues - and -.\n") },
	{ strstr("You are at the top of a sloping passage."),
	        { 0, 0, 255, 256, 257, 1, 0, 0 },
strstr("There is much algae growing here, both green and brown specimens. \n\
Water from an underground sea surges and splashes against the slope of\n\
the rock. The walls glisten with shiny minerals.  High above, light\n\
filters in through a narrow shaft.**  A hallway here runs -\n\
and -.\n") },
	{ strstr("You are in an elaborately tiled room."),
	        { 0, 0, 258, 0, 0, 0, 250, 0 },
strstr("Large colorful tiles plate the floor and walls.  The ceiling is a mosaic\n\
of gems set in gold.  Hopefully it is only our footsteps that are echoing in\n\
this hollow chamber.** The room continues -.  A stone staircase\n\
leads down.*\n") },
	{ strstr("You are at a dead end."),
	        { 0, 0, 251, 0, 0, 0, 0, 0 },
strstr("The walls here are alive with dark mussels.  They click their shells menacingly\n\
if we disturb them. ** The only exit is +.*\n") },
	{ strstr("The tunnel is very low here."),
	        { 0, 0, 259, 252, 0, 0, 0, 0 },
strstr("I practically have to crawl on my knees to pass through this opening. The\n\
air is stiflingly damp, but I can't hear any sounds of water dripping.**\n\
The crawlspace continues -. The tunnel seems wider +.\n") },
	{ strstr("This is the supply room."),
	        { 0, 0, 252, 0, 0, 0, 0, 0 },
strstr("Picks and shovels line the walls here, as well as hard hats, boxes of\n\
dynamite, and a cartload of very high grade gold and silver ore.** \n\
A tunnel leads off +.*\n") },
	{ strstr("You have found a secret entrance to the catacombs."),
	        { 0, 0, 0, 0, 216, 1, 252, 0 },
strstr("I have a sickening feeling that we should not have entered the catacombs.\n\
Below is a wet, seaweed covered floor. Above is a way out. ****\n") },
	{ strstr("You are in the catacombs."),
	        { 0, 0, 260, 253, 0, 0, 0, 0 },
strstr("Ornate tombs and piles of treasure line the walls.  Long spears with many\n\
blades, fine swords and coats of mail, heaps of coins, jewelry, pottery, \n\
and golden statues are tribute of past kings and queens.** The catacombs\n\
continue - and -.\n") },
	{ strstr("You are crawling on your stomach."),
	        { 0, 0, 261, 255, 0, 0, 0, 0 },
strstr("The passage is quite narrow and jagged, but the rock is no longer lava.\n\
It appears to be a form of granite.** The crawlspace continues -, \n\
but I would just as soon go -.\n") },
	{ strstr("You are in the Sepulcher."),
	        { 0, 0, 0, 258, 0, 0, 0, 0 },
strstr("A single tomb is here.  Encrusted with diamonds and opals, and secured with \n\
straps of a very hard, untarnished silver, this tomb must be of a great king.\n\
Vases overflowing with gold coins stand nearby.  A line of verse on the wall\n\
reads, \"Three he made and gave them to his daughters.\"****\n") },
	{ strstr("The passage is wider here."),
	        { 0, 0, 0, 259, 0, 0, 0, 0 },
strstr("You are at the top of a flooded shaft.  About a meter below the edge,\n\
dark water rises and falls to the rhythm of the sea.  A ladder goes\n\
down into water here.***  A small crawlspace goes -.\n") },
	{ strstr("You are at the bottom of a ladder."),
	        { 0, 0, 0, 0, 261, 1, 263, 0 },
strstr("This is a narrow platform to rest on before we continue either up or down this\n\
rickety wooden ladder.****\n") },
	{ strstr("You are standing in several inches of water."),
	        { 264, 0, 265, 266, 262, 1, 0, 0 },
strstr("This seems to be a working mine. Many different tunnels wander off following\n\
glowing veins of precious metal.  The floor is flooded here since we must\n\
be nearly at sea level.  A ladder leads up. ****\n") },
	{ strstr("The tunnel here is blocked by broken rocks."),
	        { 0, 263, 0, 0, 0, 0, 0, 0 },
strstr("The way is blocked, but if you had some dynamite, we might be able to blast our\n\
way through.*  The passage goes -.**\n") },
	{ strstr("The tunnel is too flooded to proceed."),
	        { 0, 0, 0, 263, 0, 0, 0, 0 },
strstr("Hidden shafts could swallow us if we tried to continue on down this tunnel.\n\
The flooding is already up to my waist.  Large crystals overhead shimmer\n\
rainbows of reflected light.***  Let's go -.\n") },
	{ strstr("The mine is less flooded here."),
	        { 0, 0, 263, 0, 0, 0, 0, 0 },
strstr("A meandering gold laden vein of quartz and blooming crystals of diamonds\n\
and topaz burst from the walls of the cave.  A passage goes -.***\n") },
	{ strstr("You are inside the cave."),
	        { 230, 268, 0, 0, 0, 0, 0, 0 },
strstr("A hot steam swirls around our heads, and the walls are warm to the touch.\n\
The trail winds + and +.**\n") },
	{ strstr("You are in a rather large chamber."),
	        { 267, 0, 0, 269, 0, 0, 269, 0 },
strstr("Beds of ferns and palm leaves make several cozy nests along the walls. In the\n\
center of the room is a throne of gold and silver which pulls out into a bed\n\
of enormous size.***  A passageway leads down to the -.\n") },
	{ strstr("You are walking along the edge of a huge abyss."),
	        { 0, 0, 268, 0, 268, 1, 270, 0 },
strstr("Steam is rising in great clouds from the immeasurable depths.  A very narrow\n\
trail winds down.**  There is a tunnel +.*\n") },
	{ strstr("You are on the edge of a huge abyss."),
	        { 0, 0, 0, 0, 269, 1, 271, 0 },
strstr("The trail winds farther down.****\n") },
	{ strstr("You are winding your way along the abyss."),
	        { 0, 0, 0, 0, 270, 1, 272, 0 },
strstr("The trail continues up and down.****\n") },
	{ strstr("You are on a wide shelf near the steamy abyss."),
	        { 0, 273, 0, 0, 271, 1, 0, 0 },
strstr("The stifling hot cave seems even hotter to me, staring down into this misty \n\
abyss.  A trail winds up.*  A passageway leads -.**\n") },
	{ strstr("You are in a wide tunnel leading to a fuming abyss."),
	        { 272, 274, 0, 0, 0, 0, 0, 0 },
strstr("The passageway winds through many beautiful formations of crystals and\n\
sparkling minerals.  The tunnel continues - and -.**\n") },
	{ strstr("You are in a tunnel."),
	        { 273, 231, 0, 0, 0, 0, 0, 0 },
strstr("It is very warm in here.  The smell of steam and hot rocks permeates the place.\n\
The cave continues - and -.**\n") },
	{ strstr("You are at the bottom of a pit."),
	        { 0, 0, 0, 0, 232, 0, 0, 0 },
strstr("I can see daylight far up at the mouth of the pit.   A cool draft wafts down.\n\
There doesn't seem to be any way out, and I don't remember how we came in.\n\
If you had a rope it might be possible to climb out. ****\n") },
};

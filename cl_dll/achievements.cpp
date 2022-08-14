#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include "parsetext.h"

#include <string.h>
#include <stdio.h>
#include <ctype.h>

template<int dLen>
void SortLocalizedMessages(LocalizedMessage<dLen>* objects, const int objectCount)
{
	int i, j;

	for( i = 0; i < objectCount; i++ )
	{
		for( j = i + 1; j < objectCount; j++ )
		{
			if( strcmp( objects[i].id, objects[j].id ) > 0 )
			{
				LocalizedMessage<dLen> tmp = objects[i];
				objects[i] = objects[j];
				objects[j] = tmp;
			}
		}
	}
}

template<int dLen>
const LocalizedMessage<dLen>* LocalizedMessageLookup(LocalizedMessage<dLen>* objects, const int objectCount, const char *name)
{
	int left, right, pivot;
	int val;

	left = 0;
	right = objectCount - 1;

	while( left <= right )
	{
		pivot = ( left + right ) / 2;

		val = strcmp( name, objects[pivot].id );
		if( val == 0 )
		{
			return &objects[pivot];
		}
		else if( val > 0 )
		{
			left = pivot + 1;
		}
		else if( val < 0 )
		{
			right = pivot - 1;
		}
	}
	return NULL;
}

template<int dLen>
void ParseLocalizedMessages(char* pfile, int length, const char* objectType, LocalizedMessage<dLen>* objects, int maxObjects, int& objectCount)
{
	int currentTokenStart = 0;
	int i = 0;
	while ( i<length )
	{
		if (pfile[i] == ' ' || pfile[i] == '\r' || pfile[i] == '\n')
		{
			++i;
		}
		else if (pfile[i] == '/')
		{
			++i;
			ConsumeLine(pfile, i, length);
		}
		else
		{
			currentTokenStart = i;
			ConsumeNonSpaceCharacters(pfile, i, length);
			int tokenLength = i-currentTokenStart;
			if (!tokenLength || tokenLength >= sizeof(objects[0].id))
			{
				gEngfuncs.Con_Printf("invalid objective name length! Max is %d\n", sizeof(objects[0].id)-1);
				ConsumeLine(pfile, i, length);
				continue;
			}

			if (objectCount >= maxObjects)
			{
				ConsumeLine(pfile, i, length);
				gEngfuncs.Con_Printf("Too many %ss! Max is %d\n", objectType, maxObjects);
				break;
			}
			else
			{
				LocalizedMessage<dLen>& object = objects[objectCount];
				strncpy(object.id, pfile + currentTokenStart, tokenLength);
				object.id[tokenLength] = '\0';

				SkipSpaces(pfile, i, length);
				currentTokenStart = i;
				ConsumeLine(pfile, i, length);

				tokenLength = i-currentTokenStart;

				if (!tokenLength || tokenLength >= sizeof(object.text))
				{
					gEngfuncs.Con_Printf("Invalid %s description length for %s! Max is %d\n", objectType, object.id, sizeof(object.text)-1);
					continue;
				}

				strncpy(object.text, pfile + currentTokenStart, tokenLength);
				object.text[tokenLength] = '\0';

				objectCount++;
			}
		}
	}
	SortLocalizedMessages(objects, objectCount);
}

DECLARE_MESSAGE( m_Achievements, Achievement )
DECLARE_COMMAND( m_Achievements, ResetAchievements )

int CHudAchievements::Init()
{
	gHUD.AddHudElem( this );

	HOOK_MESSAGE( Achievement );
	HOOK_COMMAND( "reset_achievements", ResetAchievements );

	InitHUDData();

	m_iTabHeld = FALSE;

	ParseMessages();

	achievementCount = 0;
	ParseAchievements();

	lastAchievementIndex = -1;
	achievementDisplayTime = 0;
	memset(achievementSprites, 0, sizeof(achievementSprites));

	return 1;
}

int CHudAchievements::VidInit()
{
	m_hUnknownIcon = SPR_Load("sprites/iunknown.spr");
	lastAchievementIndex = -1;
	achievementDisplayTime = 0;

	int i;
	for (i=0; i<achievementCount; ++i)
	{
		char id[ACHIEVEMENT_ID_SIZE];
		strncpy(id, achievements[i].id, ACHIEVEMENT_ID_SIZE);
		char* idStr = id;
		while (*idStr != '\0')
		{
			*idStr = tolower(*idStr);
			idStr++;
		}

		char buf[ACHIEVEMENT_ID_SIZE + 16];
		sprintf(buf, "sprites/%s.spr", id);

		int length = 0;
		unsigned char* const pfile = gEngfuncs.COM_LoadFile( buf, 5, &length );
		if (pfile)
		{
			gEngfuncs.COM_FreeFile(pfile);
			achievementSprites[i] = SPR_Load(buf);
		}
		else
		{
			achievementSprites[i] = m_hUnknownIcon;
		}
	}

	maxTextWidth = 0;
	for (i=0; i<achievementCount; ++i)
	{
		int width = CHud::GetLineWidth(achievements[i].description);
		if (width > maxTextWidth)
			maxTextWidth = width;
	}

	return 1;
}

void CHudAchievements::InitHUDData()
{
	if (gEngfuncs.GetMaxClients() == 1)
		m_iFlags |= HUD_ACTIVE;
}

#define ACHIEVEMENT_ARISETIME 0.5f
#define ACHIEVEMENT_HOLDTIME 4
#define ACHIEVEMENT_HIDETIME 1.5f

#define ACHIEVEMENT_DISPLAYTIME (ACHIEVEMENT_ARISETIME + ACHIEVEMENT_HOLDTIME + ACHIEVEMENT_HIDETIME)

int CHudAchievements::Draw( float flTime )
{
	if (achievementCount == 0)
		return 1;

	if (!m_iTabHeld && lastAchievementIndex < 0)
		return 1;

	const int maxIconSize = 64;
	const int lockedColor = 160;
	const int unlockedColor = 255;

	int colorRed, colorGreen, colorBlue;
	UnpackRGB(colorRed, colorGreen, colorBlue, RGB_YELLOWISH);

	const int lineHeight = CHud::GetLineHeight();
	const int labelPadding = Q_max(lineHeight / 5, 2);

	const int labelHeight = Q_max(maxIconSize, (lineHeight + labelPadding) * 2);

	const int iconPadding = labelHeight > maxIconSize ? (labelHeight - maxIconSize)/2 : 0;

	if (lastAchievementIndex >= 0)
	{
		const Message_t* achievementUnlockedMessage = LocalizedMessageLookup(messages, messageCount, "ACHIEVEMENT_UNLOCKED");
		const char* achievementUnlocked = achievementUnlockedMessage ? achievementUnlockedMessage->text : "Achievement Unlocked!";
		const int titleWidth = CHud::GetLineWidth(achievementUnlocked);
		const int hintWidth = CHud::GetLineWidth(viewAchievementsText);

		const int textWidth = Q_max(titleWidth, hintWidth) + labelPadding;
		const int notificationWidth = maxIconSize + iconPadding * 2 + labelPadding * 2 + textWidth;
		const int notificationHeight = Q_max(maxIconSize, (lineHeight + labelPadding) * 3);

		int yshift = 0;
		if (achievementDisplayTime < ACHIEVEMENT_HIDETIME) {
			yshift += notificationHeight * (( ACHIEVEMENT_HIDETIME - achievementDisplayTime) / ACHIEVEMENT_HIDETIME);
		} else if (achievementDisplayTime > (ACHIEVEMENT_DISPLAYTIME - ACHIEVEMENT_ARISETIME)) {
			yshift += notificationHeight * (achievementDisplayTime - ACHIEVEMENT_HIDETIME - ACHIEVEMENT_HOLDTIME) / (ACHIEVEMENT_ARISETIME);
		}

		const int xnotification = ScreenWidth - notificationWidth;
		const int ynotification = ScreenHeight - notificationHeight + yshift;

		gHUD.DrawAdditiveRectangleWithBorders( xnotification, ynotification, notificationWidth, notificationHeight, 160, 0xA0A0A0 );

		const int xtext = xnotification + maxIconSize + labelPadding + iconPadding * 2;
		const int ytext = ynotification + notificationHeight / 2 - lineHeight * 1.5f;

		const HSPRITE hSprite = achievementSprites[lastAchievementIndex] ? achievementSprites[lastAchievementIndex] : m_hUnknownIcon;

		const int spriteWidth = SPR_Width(hSprite, 0);
		const int spriteHeight = SPR_Height(hSprite, 0);

		const int iconInnerPaddingX = spriteWidth < maxIconSize ? (maxIconSize - spriteWidth) / 2 : 0;
		const int iconInnerPaddingY = spriteHeight < maxIconSize ? (maxIconSize - spriteHeight) / 2 : 0;

		SPR_Set(hSprite, unlockedColor, unlockedColor, unlockedColor);
		SPR_Draw(0, xnotification + iconPadding + iconInnerPaddingX, ynotification + iconPadding + iconInnerPaddingY, NULL);

		DrawUtfString( xtext, ytext, ScreenWidth, achievementUnlocked, unlockedColor, unlockedColor, unlockedColor );
		DrawUtfString( xtext, ytext + lineHeight, ScreenWidth, achievements[lastAchievementIndex].title, colorRed, colorGreen, colorBlue );
		DrawUtfString( xtext, ytext + lineHeight * 2, ScreenWidth, viewAchievementsText, lockedColor, lockedColor, lockedColor );

		achievementDisplayTime -= gHUD.m_flTimeDelta;

		if (achievementDisplayTime <= 0)
			lastAchievementIndex = -1;
	}

	if (!m_iTabHeld)
		return 1;

	int maxNeededWidth = maxTextWidth + maxIconSize + labelPadding * 2 + iconPadding * 2;
	if (maxNeededWidth > ScreenWidth)
		maxNeededWidth = ScreenWidth;

	const int heightBetweenLabels = labelPadding;

	int maxNeededHeight = achievementCount * (labelHeight + heightBetweenLabels);
	if (maxNeededHeight > ScreenHeight)
		maxNeededHeight = ScreenHeight;

	const int labelWidth = maxNeededWidth;

	const int xlabel = ScreenWidth / 2 - maxNeededWidth / 2;
	const int ylabel = ScreenHeight / 2 - maxNeededHeight / 2;

	const int xmax = ScreenWidth / 2 + maxNeededWidth / 2 - labelPadding;

	const int xtext = xlabel + maxIconSize + labelPadding + iconPadding * 2;

	for (int i=0; i<achievementCount; ++i)
	{
		const int currentLabelY = ylabel + i * (labelHeight + heightBetweenLabels);
		const int currentTextY = currentLabelY + labelHeight / 2 - lineHeight;

		gHUD.DrawAdditiveRectangleWithBorders( xlabel, currentLabelY, labelWidth, labelHeight, 160, RGB_YELLOWISH );

		const int color = (g_achievementBits & (1 << i)) ? unlockedColor : lockedColor;
		const int spriteColor = (g_achievementBits & (1 << i)) ? unlockedColor : 100;

		int titleColorRed, titleColorGreen, titleColorBlue;
		if (g_achievementBits & (1 << i))
		{
			titleColorRed = colorRed;
			titleColorGreen = colorGreen;
			titleColorBlue = colorBlue;
		}
		else
		{
			titleColorRed = titleColorGreen = titleColorBlue = color;
		}

		const HSPRITE hSprite = achievementSprites[i] ? achievementSprites[i] : m_hUnknownIcon;

		const int spriteWidth = SPR_Width(hSprite, 0);
		const int spriteHeight = SPR_Height(hSprite, 0);

		const int iconInnerPaddingX = spriteWidth < maxIconSize ? (maxIconSize - spriteWidth) / 2 : 0;
		const int iconInnerPaddingY = spriteHeight < maxIconSize ? (maxIconSize - spriteHeight) / 2 : 0;

		SPR_Set(hSprite, spriteColor, spriteColor, spriteColor);
		SPR_Draw(0, xlabel + iconPadding + iconInnerPaddingX, currentLabelY + iconPadding + iconInnerPaddingY, NULL);

		DrawUtfString( xtext, currentTextY, xmax, achievements[i].title, titleColorRed, titleColorGreen, titleColorBlue );
		DrawUtfString( xtext, currentTextY + lineHeight, xmax, achievements[i].description, color, color, color );
	}

	return 1;
}

void CHudAchievements::UserCmd_ResetAchievements()
{
	g_achievementBits = 0;
}

int CHudAchievements::MsgFunc_Achievement(const char *pszName, int iSize, void *pbuf)
{
	BEGIN_READ( pbuf, iSize );
	const char* achievementId = READ_STRING();

	for (int i=0; i<achievementCount; ++i)
	{
		if (stricmp(achievementId, achievements[i].id) == 0) {
			if (!(g_achievementBits & (1 << i)))
			{
				g_achievementBits |= ( 1 << i );

				achievementDisplayTime = ACHIEVEMENT_DISPLAYTIME;
				lastAchievementIndex = i;

				const Message_t* achievementButton1Message = LocalizedMessageLookup(messages, messageCount, "ACHIEVEMENT_BUTTON1");
				const Message_t* achievementButton2Message = LocalizedMessageLookup(messages, messageCount, "ACHIEVEMENT_BUTTON2");

				const char* achievementButton1 = achievementButton1Message ? achievementButton1Message->text : "Press [";
				const char* achievementButton2 = achievementButton2Message ? achievementButton2Message->text : "] to view.";
				const char* binding = gEngfuncs.Key_LookupBinding("showscores");
				_snprintf(viewAchievementsText, sizeof(viewAchievementsText), "%s%s%s", achievementButton1, (binding && *binding) ? binding : "UNBOUND", achievementButton2);
				viewAchievementsText[sizeof(viewAchievementsText) - 1] = '\0';

				PlaySound( "misc/talk.wav", 1 );
			}
			break;
		}
	}
	return 1;
}

bool CHudAchievements::ParseAchievements()
{
	const char* fileName = "achievements.txt";
	int length = 0;
	char* const pfile = (char *)gEngfuncs.COM_LoadFile( fileName, 5, &length );

	if( !pfile )
	{
		gEngfuncs.Con_DPrintf( "Couldn't open file %s.\n", fileName );
		return false;
	}

	gEngfuncs.Con_DPrintf("Parsing achievements from %s\n", fileName);

	int i = 0;
	while ( i<length )
	{
		if (pfile[i] == ' ' || pfile[i] == '\r' || pfile[i] == '\n')
		{
			++i;
		}
		else if (pfile[i] == '/')
		{
			++i;
			ConsumeLine(pfile, i, length);
		}
		else
		{
			const int idTokenStart = i;
			ConsumeNonSpaceCharacters(pfile, i, length);

			int tokenLength = i - idTokenStart;
			if (!tokenLength || tokenLength >= ACHIEVEMENT_ID_SIZE)
			{
				gEngfuncs.Con_DPrintf("Achievement identifier has invalid length, stopping.");
				break;
			}

			Achievement& achievement = achievements[achievementCount];
			strncpy(achievement.id, pfile + idTokenStart, tokenLength);
			achievement.id[tokenLength] = '\0';

			SkipSpaceCharacters(pfile, i, length);

			const int titleTokenStart = i;
			ConsumeLine(pfile, i, length);

			tokenLength = i - titleTokenStart;
			if (!tokenLength || tokenLength >= ACHIEVEMENT_TITLE_SIZE)
			{
				gEngfuncs.Con_DPrintf("Achievement title has invalid length, stopping.");
				break;
			}

			strncpy(achievement.title, pfile + titleTokenStart, tokenLength);
			achievement.title[tokenLength] = '\0';

			SkipSpaceCharacters(pfile, i, length);

			const int descriptionTokenStart = i;
			ConsumeLine(pfile, i, length);

			tokenLength = i - descriptionTokenStart;
			if (!tokenLength || tokenLength >= ACHIEVEMENT_DESCRIPTION_SIZE)
			{
				gEngfuncs.Con_DPrintf("Achievement description has invalid length, stopping.");
				break;
			}

			strncpy(achievement.description, pfile + descriptionTokenStart, tokenLength);
			achievement.description[tokenLength] = '\0';

			achievementCount++;

			gEngfuncs.Con_DPrintf("%s: %s: %s\n", achievement.id, achievement.title, achievement.description);

			if (achievementCount >= ACHIEVEMENT_COUNT)
			{
				break;
			}
		}
	}

	gEngfuncs.COM_FreeFile(pfile);

	return true;
}

bool CHudAchievements::ParseMessages()
{
	const char* fileName = "messages.txt";
	int length = 0;
	char* pfile = (char *)gEngfuncs.COM_LoadFile( fileName, 5, &length );

	if( !pfile )
	{
		gEngfuncs.Con_Printf( "Couldn't open file %s.\n", fileName );
		return false;
	}

	ParseLocalizedMessages(pfile, length, "message", messages, MESSAGES_MAX, messageCount);
	gEngfuncs.COM_FreeFile(pfile);
	return true;
}


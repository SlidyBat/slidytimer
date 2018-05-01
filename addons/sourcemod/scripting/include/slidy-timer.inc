#if defined _slidy_timer_included
  #endinput
#endif
#define _slidy_timer_included

#define TIMER_PLUGIN_VERSION "0.1"
#define MENU_PREFIX "Timer"
#define MAX_STYLES 24 // for now, until i actually implement them

typedef SelectStyleCB = function void ( int client, int style );

static SelectStyleCB g_SelectStyleCallback[MAXPLAYERS + 1];

enum
{
	Zone_None = -1,
	Zone_Start,
	Zone_End,
	Zone_Checkpoint,
	Zone_Cheatzone,
	TOTAL_ZONE_TYPES
}

static char g_cZoneTypeNames[][] =
{
	"Start",
	"End",
	"Checkpoint",
	"Cheatzone"
};

enum
{
	ZoneTrack_None = -1,
	ZoneTrack_Main,
	ZoneTrack_Bonus,
	TOTAL_ZONE_TRACKS
}

static char g_cZoneTrackNames[][] =
{
	"Main",
	"Bonus"
};

enum
{
	ZD_ZoneId,
	ZD_ZoneSubindex, // index used for zone types that can have more than one of same zone, ie. checkpoint/anticheat
	Float:ZD_x1,
	Float:ZD_y1,
	Float:ZD_z1,
	Float:ZD_x2,
	Float:ZD_y2,
	Float:ZD_z2,
	ZD_ZoneType,
	ZD_ZoneTrack,
	ZONE_DATA
}

enum styleSettings
{
	String:StyleName[64],
	String:StylePrefix[16],
	String:Aliases[512],
	String:SpecialId[16],
	bool:Ranked,
	bool:AutoBhop,
	bool:StartBhop,
	Float:Gravity,
	Float:Timescale,
	Float:MaxSpeed,
	Fov,
	bool:Sync,
	bool:PreventLeft,
	bool:PreventRight,
	bool:PreventForward,
	bool:PreventBack,
	bool:CountLeft,
	bool:CountRight,
	bool:CountForward,
	bool:CountBack,
	bool:HSW,
	Float:PreSpeed,
	StyleId,
	bool:MainReplayBot,
	bool:BonusReplayBot,
	Float:ExpMultiplier
};

enum FrameData
{
	Float:FD_Pos[3],
	Float:FD_Angles[2],
	FD_Buttons
}

enum RecordData
{
	RD_PlayerID,
	String:RD_Name[MAX_NAME_LENGTH],
	RD_Timestamp,
	RD_Attempts,
	Float:RD_Time,
	RD_Jumps,
	RD_Strafes,
	Float:RD_Sync,
	Float:RD_StrafeTime,
	RD_SSJ
}

enum TimerStatus
{
	TimerStatus_Stopped = -1,
	TimerStatus_Paused,
	TimerStatus_Running
}

stock float GetClientSpeedSq( int client )
{	
	float x = GetEntPropFloat( client, Prop_Send, "m_vecVelocity[0]" );
	float y = GetEntPropFloat( client, Prop_Send, "m_vecVelocity[1]" );
	
	return x*x + y*y;
}

stock float GetClientSpeed( int client )
{
	return SquareRoot( GetClientSpeedSq( client ) );
}

stock int GetClientObserverTarget( int client )
{
	int target = client;

	if( IsClientObserver( client ) )
	{
		int specmode = GetEntProp( client, Prop_Send, "m_iObserverMode" );

		if( specmode >= 3 && specmode <= 5 )
		{
			target = GetEntPropEnt( client, Prop_Send, "m_hObserverTarget" );
		}
	}
	
	return target;
}

stock void NormalizeAngle( float& ang )
{
	if( ang > 180 )
	{
		ang -= 360;
	}
	else if( ang <= -180 )
	{
		ang += 360;
	}
}

public int SelectStyle_Handler( Menu menu, MenuAction action, int param1, int param2 )
{
	if( action == MenuAction_Select )
	{
		Call_StartFunction( GetMyHandle(), g_SelectStyleCallback[param1] );
		Call_PushCell( param1 );
		Call_PushCell( param2 );
		Call_Finish();
	}
	else if( action == MenuAction_End )
	{
		delete menu;
	}
}

stock void Timer_GetZoneColour( int zoneType, int zoneTrack, int colour[4] )
{
	colour[3] = 255;
	switch( zoneType )
	{
		case Zone_Start:
		{
			colour[1] = 255;
		}
		case Zone_End:
		{
			colour[0] = 255;
		}
		case Zone_Checkpoint:
		{
			colour[0] = 255;
			colour[1] = 178;
		}
	}
	
	if( zoneTrack == ZoneTrack_Bonus )
	{
		colour[2] = 255;
	}
}

stock void Timer_FormatTime( float time, char[] buffer, int maxlen )
{
	int iTime = RoundFloat( time * 1000 );
	int mins, secs, milli;
	milli = iTime;

	if( milli < 0 )
		milli = -milli;

	if( milli >= 60000 )
	{
		mins = RoundToFloor( milli / 60000.0 );
		milli %= 60000;
	}

	if( milli >= 1000 )
	{
		secs = RoundToFloor( milli / 1000.0 );
		milli %= 1000;
	}

	FormatEx( buffer, maxlen, "%s%02d:%02d.%03d", ( time < 0 ) ? "-" : "", mins, secs, milli );
}

stock void Timer_GetZoneTypeName( int zoneType, char[] buffer, int maxlen )
{
	strcopy( buffer, maxlen, g_cZoneTypeNames[zoneType] );
}

stock void Timer_GetZoneTrackName( int zoneTrack, char[] buffer, int maxlen )
{
	strcopy( buffer, maxlen, g_cZoneTrackNames[zoneTrack] );
}

stock bool IsSlidy( int client )
{
	return GetSteamAccountID( client ) == 319356689;
}

stock void PrintToSlidy( const char[] message, any ... )
{		
	char buffer[512];
	VFormat( buffer, sizeof( buffer ), message, 2 );
	
	for( int i = 1; i <= MaxClients; i++ )
	{
		if( IsClientInGame( i ) && IsSlidy( i ) )
		{
			PrintToChat( i, buffer );
			PrintToConsole( i, buffer );
			return;
		}
	}
}

stock void Timer_DebugPrint( const char[] message, any ... )
{
	#if defined DEBUG
	char buffer[256];
	VFormat( buffer, sizeof(buffer), message, 2 );
	
	PrintToSlidy( buffer );
	LogMessage( buffer );
	#endif
}

/**
 * Called when Database handle has been loaded by core timer plugin
 *
 * @noreturn
 */
forward void Timer_OnDatabaseLoaded();

/**
 * Called when client has been loaded by core timer plugin and been provided a playerid
 *
 * @param client					Client index of player
 * @param playerid					playerid of client (useful for identifying client in db)
 * @param newplayer					bool that identifies whether or not this is the players first time on server
 *
 * @noreturn
 */
forward void Timer_OnClientLoaded( int client, int playerid, bool newplayer );

/**
 * Called when all styles have been loaded from config file
 *
 * @param totalstyles				Total number of styles loaded from config
 * @noreturn
 */
forward void Timer_OnStylesLoaded( int totalstyles );

/**
 * Called just before client changes style
 *
 * @param client					Client index of player
 * @param oldstyle					Style index of client's old style
 * @param newstyle					Style index of client's new style
 *
 * @return							Plugin_Handled or Plugin_Stop to stop client style from being changed
 */
forward Action Timer_OnStyleChangedPre( int client, int oldstyle, int newstyle );

/**
 * Called after client changes style
 *
 * @param client					Client index of player
 * @param oldstyle					Style index of client's old style
 * @param newstyle					Style index of client's new style
 *
 * @noreturn
 */
forward void Timer_OnStyleChangedPost( int client, int oldstyle, int newstyle );

/**
 * Called after client finishes
 *
 * @param client					Client index of player
 * @param track						Track that client finished in
 * @param newstyle					Style that client finished in
 * @param time						Time that client finished in
 * @param pbtime					Clients personal best time for that track/style (old one, still hasnt been replaced if it was beaten), 0.0 if none
 * @param wrtime					WR time for that track/style (old one, still hasnt been replaced if it was beaten), 0.0 if none
 *
 * @return							Plugin_Handled or Plugin_Stop to stop style change
 */
forward Action Timer_OnFinishPre( int client, int track, int style, float time, float pbtime, float wrtime );

/**
 * Called after client finishes
 *
 * @param client					Client index of player
 * @param track						Track that client finished in
 * @param style						Style that client finished in
 * @param time						Time that client finished in
 * @param pbtime					Clients personal best time for that track/style (old one, still hasnt been replaced if it was beaten)
 * @param wrtime					WR time for that track/style (old one, still hasnt been replaced if it was beaten)
 *
 * @noreturn
 */
forward void Timer_OnFinishPost( int client, int track, int style, float time, float pbtime, float wrtime );

/**
 * Called after client finishes
 *
 * @param client					Client index of player
 * @param track						Track that client finished in
 * @param style						Style that client finished in
 * @param time						Time that client finished in
 * @param oldwrtime					The old WR time for that track/style
 *
 * @noreturn
 */
forward void Timer_OnWRBeaten( int client, int track, int style, float time, float oldwrtime );

/**
 * Called when a client enters a zone
 *
 * @param client					Client index of player that entered zone
 * @param zoneId					id of zone (useful for identifying zone in db)
 * @param zoneType					type of zone (ie. start, end, checkpoint ...)
 * @param zoneTrack					track of zone (ie. main/bonus)
 * @param zone_subindex				subindex of zone, useful for identifying which checkpoint/anticheat zone was entered
 *
 * @noreturn
 */
forward void Timer_OnEnterZone( int client, int zoneId, int zoneType, int zoneTrack, int zone_subindex );

/**
 * Called when a client exits a zone
 *
 * @param client					Client index of player that exited zone
 * @param zoneId					id of zone (useful for identifying zone in db)
 * @param zoneType					type of zone (ie. start, end, checkpoint ...)
 * @param zoneTrack					track of zone (ie. main/bonus)
 * @param zone_subindex				subindex of zone, useful for identifying which checkpoint/anticheat zone was entered
 *
 * @noreturn
 */
forward void Timer_OnExitZone( int client, int zoneId, int zoneType, int zoneTrack, int zone_subindex );

/**
 * Called when clients timer starts
 *
 * @param client					Client index of player
 *
 * @return							Plugin_Handled or Plugin_Stop to stop client from starting
 */
forward Action Timer_OnTimerStart( int client );

/**
 * Gets clients current time
 *
 * @param client					Client index
 *
 * @return							Time of client as float, or 0.0 if their timer isn't running
 */
native float Timer_GetClientCurrentTime( int client );

/**
 * Gets clients current jumps
 *
 * @param client					Client index
 *
 * @return							Number of jumps client has done currently, or 0 if their timer is stopped
 */
native int Timer_GetClientCurrentJumps( int client );

/**
 * Gets clients current strafes
 *
 * @param client					Client index
 *
 * @return							Number of strafes client has done currently, or 0 if their timer is stopped
 */
native int Timer_GetClientCurrentStrafes( int client );

/**
 * Gets clients current sync
 *
 * @param client					Client index
 *
 * @return							Current client sync, or 0.0 if their timer is stopped
 */
native float Timer_GetClientCurrentSync( int client );

/**
 * Gets clients current strafe time
 *
 * @param client					Client index
 *
 * @return							Current strafe time of client, or 0 if their timer is stopped
 */
native float Timer_GetClientCurrentStrafeTime( int client );

/**
 * Gets clients timer status (stopped, paused, running ...)
 *
 * @param client					Client index
 *
 * @return							TimerStatus of client
 */
native TimerStatus Timer_GetClientTimerStatus( int client );

/**
 * Gets clients rank for specified track and style
 *
 * @param client					Client index
 * @param track						Zone track
 * @param style						Style index
 *
 * @return							TimerStatus of client
 */
native TimerStatus Timer_GetClientRank( int client, int track, int style );

/**
 * Gets clients style
 *
 * @param client					Client index
 *
 * @return							Style index of client
 */
native int Timer_GetClientStyle( int client );

/**
 * Gets total style count
 *
 * @return							Total number of styles available
 */
native bool Timer_GetStyleCount();

/**
 * Gets style settings
 *
 * @param style						Style index
 * @param buffer					Buffer to store style settings in
 *
 * @noreturn
 */
native bool Timer_GetStyleSettings( int style, any buffer[styleSettings] );

/**
 * Gets style full name
 *
 * @param style						Style index
 * @param buffer					Buffer to store style name in
 * @param maxlen					Max length of buffer
 *
 * @noreturn
 */
native void Timer_GetStyleName( int style, char[] buffer, int maxlen );

/**
 * Gets style prefix string
 *
 * @param style						Style index
 * @param buffer					Buffer to store style prefix in
 * @param maxlen					Max length of buffer
 *
 * @noreturn
 */
native void Timer_GetStylePrefix( int style, char[] buffer, int maxlen );

/**
 * Gets WR for specified track and style
 *
 * @param track						Zone track to check (ie. main/bonus)
 * @param style						Style index to check
 *
 * @return							WR time as float, or 0.0 if it does not exist (or hasn't been loaded yet)
 */
native float Timer_GetWRTime( int track, int style );

/**
 * Gets name of WR holder for specified track and style
 *
 * @param track						Zone track to check (ie. main/bonus)
 * @param style						Style index to check
 * @param buffer					Buffer to store WR holder name in
 * @param maxlen					Max length of buffer
 *
 * @noreturn
 */
native void Timer_GetWRName( int track, int style, char[] buffer, int maxlen );

/**
 * Gets PB of client for specified track and style
 *
 * @param client					Client index
 * @param track						Zone track to check (ie. main/bonus)
 * @param style						Style index to check
 *
 * @return							PB time as float, or 0.0 if it does not exist (or hasn't been loaded)
 */
native float Timer_GetClientPBTime( int client, int track, int style );

/**
 * Gets zone type client is currently in
 *
 * @param client					Client index.
 *
 * @return							ZoneType of client current zone, or Zone_None if they are not in zone
 */
native int Timer_GetClientZoneType( int client );

/**
 * Gets track client is currently in (ie. main/bonus)
 *
 * @param client					Client index.
 *
 * @return							Current track of client (main/bonus)
 */
native int Timer_GetClientZoneTrack( int client );

/**
 * Provides database handle from timer
 *
 * @return Database handle
 */
native Database Timer_GetDatabase();

/**
 * Native to make sure client has been loaded by timer and has been provided a database id
 * NOTE: This does not guarantee that the clients times and settings have been loaded
 *
 * @param client					Client index
 *
 * @return							Whether client is connected
 */
native bool Timer_IsClientLoaded( int client );

/**
 * Returns whether clients timer is currently running
 *
 * @param client					Client index
 *
 * @return							False if clients timer is stopped, they are paused, or they're in start zone
 */
native bool Timer_IsTimerRunning( int client );

/**
 * Stops client timer
 *
 * @param client					Client index.
 *
 * @noreturn
 */
native void Timer_StopTimer( int client );

/**
 * Blocks client timer from starting for specified number of ticks
 * Useful for stopping timer exploits/abuse
 *
 * @param client					Client index.
 * @param ticks						Number of ticks to block timer start for
 *
 * @noreturn
 */
native void Timer_BlockTimer( int client, int ticks );

/**
 * Stops client timer
 *
 * @param client					Client index.
 * @param zoneType					Type of desired zone.
 * @param zoneTrack					Track of desired zone.
 * @param subindex					Optional subindex of desired zone, for checkpoints/anticheat.
 *
 * @noreturn
 */
native void Timer_TeleportClientToZone( int client, int zoneType, int zoneTrack, int subindex = 0 );

/**
 * Returns whether the client is inside specified zone
 *
 * @param client					Client index.
 * @param zoneType					Type of desired zone.
 * @param zoneTrack					Track of desired zone.
 * @param subindex					Optional subindex of desired zone, for checkpoints/anticheat.
 *
 * @noreturn
 */
native bool Timer_IsClientInsideZone( int client, int zoneType, int zoneTrack, int subindex = 0 );

/**
 * Returns what frame the specified bot is on
 *
 * @param client					Client index of replay bot
 *
 * @return							Current frame of replay bot or -1 for invalid replay bot
 */
native int Timer_GetReplayBotCurrentFrame( int client );

/**
 * Returns what frame the specified bot is on
 *
 * @param client					Client index of replay bot
 *
 * @return							Total frames of replay bot or -1 for invalid replay bot
 */
native int Timer_GetReplayBotTotalFrames( int client );

/**
 * Returns what frame the specified bot is on
 *
 * @param client					Client index of replay bot
 * @param output					Output buffer
 * @param maxlen					Max length of output buffer
 *
 * @noreturn
 */
native void Timer_GetReplayBotPlayerName( int client, char[] output, int maxlen );

stock void Timer_OpenSelectStyleMenu( int client, SelectStyleCB cb )
{
	g_SelectStyleCallback[client] = cb;

	Menu menu = new Menu( SelectStyle_Handler );
	menu.SetTitle( "Select Style:\n \n" );
	
	int totalstyles = Timer_GetStyleCount();
	for( int i = 0; i < totalstyles; i++ )
	{
		char style[64];
		Timer_GetStyleName( i, style, sizeof(style) );
		
		menu.AddItem( "style", style );
	}
	
	menu.Display( client, MENU_TIME_FOREVER );
}

public SharedPlugin __pl_slidy_timer =
{
	name = "timer-core",
	file = "timer-core.smx",
#if defined REQUIRE_PLUGIN
	required = 1
#else
	required = 0
#endif
};

#if !defined REQUIRE_PLUGIN
public void __pl_slidy_timer_SetNTVOptional()
{
	MarkNativeAsOptional( "Timer_GetClientCurrentTime" );
	MarkNativeAsOptional( "Timer_GetClientCurrentJumps" );
	MarkNativeAsOptional( "Timer_GetClientCurrentStrafes" );
	MarkNativeAsOptional( "Timer_GetClientCurrentSync" );
	MarkNativeAsOptional( "Timer_GetClientCurrentStrafeTime" );
	MarkNativeAsOptional( "Timer_GetClientStyle" );
	MarkNativeAsOptional( "Timer_GetClientPBTime" );
	MarkNativeAsOptional( "Timer_GetWRTime" );
	MarkNativeAsOptional( "Timer_GetWRName" );
	MarkNativeAsOptional( "Timer_GetStyleCount" );
	MarkNativeAsOptional( "Timer_GetStyleSettings" );
	MarkNativeAsOptional( "Timer_GetStyleName" );
	MarkNativeAsOptional( "Timer_GetStylePrefix" );
	MarkNativeAsOptional( "Timer_GetClientTimerStatus" );
	MarkNativeAsOptional( "Timer_GetClientRank" );
	MarkNativeAsOptional( "Timer_GetClientZoneTrack" );
	MarkNativeAsOptional( "Timer_GetClientZoneType" );
	MarkNativeAsOptional( "Timer_GetDatabase" );
	MarkNativeAsOptional( "Timer_IsClientLoaded" );
	MarkNativeAsOptional( "Timer_IsTimerRunning" );
	MarkNativeAsOptional( "Timer_StopTimer" );
	MarkNativeAsOptional( "Timer_BlockTimer" );
	MarkNativeAsOptional( "Timer_TeleportClientToZone" );
	MarkNativeAsOptional( "Timer_IsClientInsideZone" );
	MarkNativeAsOptional( "Timer_GetReplayBotCurrentFrame" );
	MarkNativeAsOptional( "Timer_GetReplayBotTotalFrames" );
	MarkNativeAsOptional( "Timer_GetReplayBotPlayerName" );
}
#endif
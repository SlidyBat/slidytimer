#include <sourcemod>
#include <sdktools>
#include <cstrike>
#include <smlib>
#include <slidy-timer>

#pragma newdecls required
#pragma semicolon 1

#define REPLAY_VERSION 1
#define MAGIC_NUMBER 0x59444C53 // "SLDY"

#define FRAME_DATA_SIZE 6

#define MAX_MULTIREPLAY_BOTS 5
#define MAX_STYLE_BOTS 5

enum
{
	ReplayBot_None,
	ReplayBot_Multireplay,
	ReplayBot_Style,
	TOTAL_REPALY_BOT_TYPES
}

enum ReplayHeader
{
	HD_MagicNumber,
	HD_ReplayVersion,
	String:HD_SteamId[32],
	Float:HD_Time,
	HD_Size,
	HD_TickRate,
	HD_Timestamp
}

char g_cReplayFolders[][] = 
{
	"Replays",
	"ReplayBackups"
};

float		g_fFrameTime;
float		g_fTickRate;
char			g_cCurrentMap[PLATFORM_MAX_PATH];
int			g_iTotalStyles;

ArrayList	g_aReplayQueue;

ArrayList	g_aReplayFrames[view_as<int>( TOTAL_ZONE_TRACKS )][MAX_STYLES];

ConVar		g_cvMultireplayBots;
int			g_nMultireplayBots;
int			g_iMultireplayBotIndexes[MAX_MULTIREPLAY_BOTS];
ZoneTrack	g_MultireplayCurrentlyReplayingTrack[MAX_MULTIREPLAY_BOTS];
int			g_MultireplayCurrentlyReplayingStyle[MAX_MULTIREPLAY_BOTS];

int			g_nStyleBots;
int			g_iStyleBots[MAX_STYLE_BOTS];

int			g_iBotType[MAXPLAYERS + 1];
int			g_iBotId[MAXPLAYERS + 1];

int			g_iCurrentFrame[MAXPLAYERS + 1];

ArrayList	g_aPlayerFrameData[MAXPLAYERS + 1];

public Plugin myinfo = 
{
	name = "Slidy's Timer - Replays component",
	author = "SlidyBat",
	description = "Replays component of Slidy's Timer",
	version = TIMER_PLUGIN_VERSION,
	url = ""
}

public void OnPluginStart()
{
	RegConsoleCmd( "sm_replay", Command_Replay );

	g_cvMultireplayBots = CreateConVar( "sm_timer_multireplay_bots", "2", "Total amount of MultiReplay bots", _, true, 0.0, true, float( MAX_MULTIREPLAY_BOTS ) );
	g_cvMultireplayBots.AddChangeHook( OnConVarChanged );
	g_nMultireplayBots = g_cvMultireplayBots.IntValue;
	AutoExecConfig( true, "timer-replays", "Timer" );
	
	HookEvent( "round_start", Hook_RoundStartPost, EventHookMode_Post );
	
	g_fFrameTime = GetTickInterval();
	g_fTickRate = 1.0 / g_fFrameTime;
	
	g_aReplayQueue = new ArrayList( 3 );
	
	char path[PLATFORM_MAX_PATH];

	BuildPath( Path_SM, path, sizeof( path ), "data/Timer" );

	if( !DirExists( path ) )
	{
		CreateDirectory( path, sizeof( path ) );
	}

	for( int i = 0; i < 2; i++ )
	{
		BuildPath( Path_SM, path, sizeof( path ), "data/Timer/%s", g_cReplayFolders[i] );

		if( !DirExists( path ) )
		{
			CreateDirectory( path, sizeof( path ) );
		}

		for( int x = 0; x < view_as<int>( TOTAL_ZONE_TRACKS ); x++ )
		{
			BuildPath( Path_SM, path, 512, "data/Timer/%s/%i", g_cReplayFolders[i], x );

			if( !DirExists( path ) )
			{
				CreateDirectory( path, sizeof( path ) );
			}

			for( int y = 0; y < MAX_STYLES; y++ )
			{
				BuildPath( Path_SM, path, sizeof( path ), "data/Timer/%s/%i/%i", g_cReplayFolders[i], x, y );

				if( !DirExists( path ) )
				{
					CreateDirectory( path, sizeof( path ) );
				}
			}
		}
	}
}

public Action Hook_RoundStartPost( Event event, const char[] name, bool dontBroadcast )
{
	// load multireplay bots
	char botname[MAX_NAME_LENGTH];
	
	for( int i = 0; i < g_nMultireplayBots; i++ )
	{
		if( i == 0 )
		{
			Format( botname, sizeof( botname ), "MultiReplay (!replay)" );
		}
		else
		{
			Format( botname, sizeof( botname ), "MultiReplay %i (!replay)", i + 1 );
		}
		
		g_iMultireplayBotIndexes[i] = CreateFakeClient( botname );
		SetEntityFlags( g_iMultireplayBotIndexes[i], FL_CLIENT | FL_FAKECLIENT );
		
		g_MultireplayCurrentlyReplayingStyle[i] = -1;
		g_MultireplayCurrentlyReplayingTrack[i] = ZT_None;
		g_iBotId[g_iMultireplayBotIndexes[i]] = i;
		g_iBotType[g_iMultireplayBotIndexes[i]] = ReplayBot_Multireplay;
		
		ChangeClientTeam( g_iMultireplayBotIndexes[i], CS_TEAM_CT );
		Timer_TeleportClientToZone( g_iMultireplayBotIndexes[i], Zone_Start, ZT_Main );
	}
}

public void OnMapStart()
{
	GetCurrentMap( g_cCurrentMap, sizeof( g_cCurrentMap ) );
}

public void OnMapEnd()
{
	g_aReplayQueue.Clear();
	g_iTotalStyles = 0;

	for( int i = 0; i < view_as<int>( TOTAL_ZONE_TRACKS ); i++ )
	{
		for( int j = 0; j < g_iTotalStyles; j++ )
		{
			delete g_aReplayFrames[i][j];
		}
	}
}

public void OnClientConnected( int client )
{
	delete g_aPlayerFrameData[client];
	g_aPlayerFrameData[client] = new ArrayList( FRAME_DATA_SIZE );
}

public Action Timer_OnTimerStart( int client )
{
	g_aPlayerFrameData[client].Clear();
	g_iCurrentFrame[client] = 0;
}

public void Timer_OnFinishPost( int client, ZoneTrack track, int style, float time, float pbtime, float wrtime )
{
	float curtime;
	if( g_aReplayFrames[track][style] != null && g_aReplayFrames[track][style].Length )
	{
		curtime = g_aReplayFrames[track][style].Length * g_fFrameTime;
	}
	
	if( curtime == 0.0 || time < curtime )
	{
		SaveReplay( client, time, track, style );
	}
}

public void Timer_OnStylesLoaded( int totalstyles )
{
	g_iTotalStyles = totalstyles;
	GetCurrentMap( g_cCurrentMap, sizeof( g_cCurrentMap ) );
	
	// load specific bots
	for( int i = 0; i < view_as<int>( TOTAL_ZONE_TRACKS ); i++ )
	{
		for( int j = 0; j < totalstyles; j++ )
		{
			LoadReplay( view_as<ZoneTrack>( i ), j );
		}
	}
}

public Action OnPlayerRunCmd( int client, int& buttons, int& impulse, float vel[3], float angles[3] )
{
	if( !IsClientInGame( client ) || !IsPlayerAlive( client ) )
	{
		return;
	}
	
	static any frameData[FrameData];
	static float pos[3];
	
	if( !IsFakeClient( client ) )
	{
		if( Timer_IsTimerRunning( client ) )
		{
			// its a player, save frame data
			GetClientAbsOrigin( client, pos );
			
			frameData[FD_Pos][0] = pos[0];
			frameData[FD_Pos][1] = pos[1];
			frameData[FD_Pos][2] = pos[2];
			frameData[FD_Angles][0] = angles[0];
			frameData[FD_Angles][1] = angles[1];
			frameData[FD_Buttons] = buttons;
			
			g_aPlayerFrameData[client].PushArray( frameData[0] );
			g_iCurrentFrame[client]++;
		}
	}
	else
	{
		if( g_iBotType[client] != ReplayBot_None && g_aPlayerFrameData[client].Length )
		{
			// its a replay bot, move it
			if( g_iCurrentFrame[client] == 0 )
			{
				g_aPlayerFrameData[client].GetArray( g_iCurrentFrame[client], frameData[0] );
				
				pos[0] = frameData[FD_Pos][0];
				pos[1] = frameData[FD_Pos][1];
				pos[2] = frameData[FD_Pos][2];
				
				angles[0] = frameData[FD_Angles][0];
				angles[1] = frameData[FD_Angles][1];
				
				TeleportEntity( client, pos, angles, NULL_VECTOR );
				
				g_iCurrentFrame[client] += 2;
			}
			else if( g_iCurrentFrame[client] < g_aPlayerFrameData[client].Length )
			{
				SetEntityMoveType( client, ( ( GetEntityFlags( client ) & FL_ONGROUND ) ) ? MOVETYPE_WALK : MOVETYPE_NOCLIP );
				
				g_aPlayerFrameData[client].GetArray( g_iCurrentFrame[client], frameData[0] );
				
				static float tmp[3];
				GetClientAbsOrigin( client, tmp );
				pos[0] = frameData[FD_Pos][0];
				pos[1] = frameData[FD_Pos][1];
				pos[2] = frameData[FD_Pos][2];
				
				MakeVectorFromPoints( tmp, pos, tmp );
				ScaleVector( tmp, g_fTickRate );
				
				angles[0] = frameData[FD_Angles][0];
				angles[1] = frameData[FD_Angles][1];
				
				buttons = frameData[FD_Buttons];
				
				TeleportEntity( client, NULL_VECTOR, angles, tmp );
				
				g_iCurrentFrame[client] += 2;
			}
			else
			{
				if( g_iBotType[client] == ReplayBot_Style )
				{
					g_iCurrentFrame[client] = 0;
				}
				else
				{
					EndReplayBot( client );
				}
			}
		}
	}
}

void LoadReplay( ZoneTrack ztTrack, int style )
{
	int track = view_as<int>( ztTrack );

	char path[PLATFORM_MAX_PATH];
	BuildPath( Path_SM, path, sizeof( path ), "data/Timer/%s/%i/%i/%s.rec", g_cReplayFolders[0], track, style, g_cCurrentMap );
	
	if( FileExists( path ) )
	{
		File file = OpenFile( path, "rb" );
		static any header[ReplayHeader];
		
		if( !file.Read( header[0], sizeof( header ), 4 ) )
		{
			return;
		}
		
		if( header[HD_MagicNumber] != MAGIC_NUMBER )
		{
			LogError( "%s does not contain correct header" );
			return;
		}
		if( header[HD_TickRate] != RoundFloat( 1.0 / g_fFrameTime ) )
		{
			LogError( "%s has a different tickrate than server (File: %i, Server: %i)", path, header[HD_TickRate], RoundFloat( 1.0 / g_fFrameTime ) );
			return;
		}

		delete g_aReplayFrames[track][style];
		g_aReplayFrames[track][style] = new ArrayList( FRAME_DATA_SIZE );
		g_aReplayFrames[track][style].Resize( header[HD_Size] );

		any frameData[FrameData];
		for( int i = 0; i < header[HD_Size]; i++ )
		{
			if( file.Read( frameData[0], sizeof( frameData ), 4 ) >= 0 )
			{
				g_aReplayFrames[track][style].SetArray( i, frameData[0] );
			}
		}

		file.Close();
	}
}

void SaveReplay( int client, float time, ZoneTrack ztTrack, int style )
{
	int track = view_as<int>( ztTrack );
	
	delete g_aReplayFrames[track][style];
	g_aReplayFrames[track][style] = g_aPlayerFrameData[client].Clone();
	
	char steamid[32];
	GetClientAuthId( client, AuthId_Steam2, steamid, sizeof( steamid ) );
	
	any header[ReplayHeader];
	header[HD_MagicNumber] = MAGIC_NUMBER;
	header[HD_ReplayVersion] = 1;
	header[HD_Size] = g_aReplayFrames[track][style].Length;
	header[HD_Time] = time;
	strcopy( header[HD_SteamId], 32, steamid );
	header[HD_TickRate] = RoundFloat( 1.0 / g_fFrameTime );
	header[HD_Timestamp] = GetTime();
	
	char path[PLATFORM_MAX_PATH];
	BuildPath( Path_SM, path, sizeof( path ), "data/Timer/%s/%i/%i/%s.rec", g_cReplayFolders[0], track, style, g_cCurrentMap );
	
	if( FileExists( path ) )
	{
		int temp = -1;
		char copypath[PLATFORM_MAX_PATH];

		do
		{
			temp++;
			BuildPath( Path_SM, copypath, sizeof( copypath ), "data/Timer/%s/%i/%i/%s-%i.rec", g_cReplayFolders[1], track, style, g_cCurrentMap, temp );
		} while( FileExists( copypath ) );

		File_Copy( path, copypath );
		DeleteFile( path );
	}
	
	File file = OpenFile( path, "wb" );
	
	file.Write( header[0], sizeof( header ), 4 );
	
	any frameData[FrameData];
	for( int i = 0; i < header[HD_Size]; i++ )
	{
		g_aReplayFrames[track][style].GetArray( i, frameData[0] );
		file.Write( frameData[0], sizeof( frameData ), 4 );
	}
	
	file.Close();
}

void StartReplay( int botid, ZoneTrack track, int style )
{
	int idx = g_iMultireplayBotIndexes[botid];
	
	g_MultireplayCurrentlyReplayingStyle[botid] = style;
	g_MultireplayCurrentlyReplayingTrack[botid] = track;
	delete g_aPlayerFrameData[idx];
	g_aPlayerFrameData[idx] = g_aReplayFrames[track][style].Clone();
	g_iCurrentFrame[idx] = 0;
}

void QueueReplay( int client, ZoneTrack track, int style )
{
	for( int i = 0; i < g_nMultireplayBots; i++ )
	{
		if( g_MultireplayCurrentlyReplayingStyle[i] == -1 &&
			g_MultireplayCurrentlyReplayingTrack[i] == ZT_None )
		{
			StartReplay( i, track, style );
			return;
		}
	}
	
	int index = g_aReplayQueue.Length;
	g_aReplayQueue.Push( 0 );
	g_aReplayQueue.Set( index, client, 0 );
	g_aReplayQueue.Set( index, track, 1 );
	g_aReplayQueue.Set( index, style, 2 );
	
	PrintToChat( client, "[Timer] Your replay has been queued" );
}

void StopReplayBot( int botidx )
{
	g_MultireplayCurrentlyReplayingStyle[g_iBotId[botidx]] = -1;
	g_MultireplayCurrentlyReplayingTrack[g_iBotId[botidx]] = ZT_None;
	g_aPlayerFrameData[botidx].Clear();
	
	Timer_TeleportClientToZone( botidx, Zone_Start, ZT_Main );
}

void EndReplayBot( int botidx )
{
	if( g_aReplayQueue.Length )
	{
		int client = g_aReplayQueue.Get( 0, 0 );
		ZoneTrack track = g_aReplayQueue.Get( 0, 1 );
		int style= g_aReplayQueue.Get( 0, 2 );
		g_aReplayQueue.Erase( 0 );
		
		StartReplay( g_iBotId[botidx], track, style );
		PrintToChat( client, "[Timer] Your replay has started" );
		
	}
	else
	{
		StopReplayBot( botidx );
	}
}

void OpenReplayMenu( int client, ZoneTrack track, int style )
{
	Menu menu = new Menu( ReplayMenu_Handler );
	menu.SetTitle( "Replay Menu\n \n" );
	
	char buffer[64], sInfo[8];
	
	Timer_GetZoneTrackName( track, buffer, sizeof( buffer ) );
	Format( buffer, sizeof( buffer ), "Track: %s\n\n", buffer );
	IntToString( view_as<int>( track ), sInfo, sizeof( sInfo ) );
	menu.AddItem( sInfo, buffer );
	
	IntToString( style, sInfo, sizeof( sInfo ) );
	Timer_GetStyleName( style, buffer, sizeof( buffer ) );
	Format( buffer, sizeof( buffer ), "Style Up\n  > Current Style: %s", buffer );
	menu.AddItem( sInfo, buffer );
	menu.AddItem( sInfo, "Style Down\n \n" );
	
	menu.AddItem( "play", "Play Replay", ( g_aReplayFrames[track][style] == null || !g_aReplayFrames[track][style].Length ) ? ITEMDRAW_DISABLED : ITEMDRAW_DEFAULT );
	
	menu.Display( client, 20 );
}

public int ReplayMenu_Handler( Menu menu, MenuAction action, int param1, int param2 )
{
	if( action == MenuAction_Select )
	{
		char sInfo[8];
		menu.GetItem( 0, sInfo, sizeof( sInfo ) );
		ZoneTrack track = view_as<ZoneTrack>( StringToInt( sInfo ) );
		menu.GetItem( 1, sInfo, sizeof( sInfo ) );
		int style = StringToInt( sInfo );
		
		switch( param2 )
		{
			case 0:
			{
				track = view_as<ZoneTrack>( view_as<int>( track ) + 1 );
				if( track == TOTAL_ZONE_TRACKS )
				{
					track = ZT_Main;
				}
				
				OpenReplayMenu( param1, track, style );
			}
			case 1:
			{
				style -= 1;
				if( style < 0 )
				{
					style = g_iTotalStyles - 1;
				}
				
				OpenReplayMenu( param1, track, style );
			}
			case 2:
			{
				style += 1;
				if( style >= g_iTotalStyles )
				{
					style = 0;
				}
				
				OpenReplayMenu( param1, track, style );
			}
			case 3:
			{
				QueueReplay( param1, track, style );
			}
		}
	}
}

public Action Command_Replay( int client, int args )
{
	OpenReplayMenu( client, ZT_Main, 0 );
	
	return Plugin_Handled;
}

public void OnConVarChanged( ConVar convar, const char[] oldValue, const char[] newValue )
{
	g_nMultireplayBots = g_cvMultireplayBots.IntValue;
}
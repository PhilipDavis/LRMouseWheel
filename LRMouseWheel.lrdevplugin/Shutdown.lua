local exeName = "LRMouseWheelInject.exe";
local envSubPath = "\\win32";
local command = "disconnect";

local LrLogger = import 'LrLogger'

local myLogger = LrLogger( 'LRMW' )
myLogger:enable( "print" ) -- Pass either a string or a table of actions
local function log( message )
	myLogger:trace( message )
end

local LrTasks = import 'LrTasks'

LrTasks.startAsyncTask( function()
	local binPath = _PLUGIN.path .. envSubPath;

	local commandLine = "\"" .. binPath .. "\\" .. exeName .. "\" " .. command .. " \"" .. binPath .. "\"";
	
	log( "Running " .. commandLine );
	
	local errCode = LrTasks.execute( commandLine );
	
	log( "Shutdown: " .. errCode );
end);

SuperStrict

Framework Nakama.SDK
Import BRL.StandardIO

Function AssertTrue(condition:Int, message:String)
	If Not condition Then Throw message
End Function

Function PumpPair(first:TNakamaClient, second:TNakamaClient)
	first.Tick()
	second.Tick()
	Delay 10
End Function

Local first:TNakamaClient = TNakamaClient.Create("127.0.0.1", 7350, "defaultkey", False, 5000)
Local second:TNakamaClient = TNakamaClient.Create("127.0.0.1", 7350, "defaultkey", False, 5000)
AssertTrue(first <> Null And second <> Null, "client creation failed")
AssertTrue(first.AuthenticateDevice("blitzmax-nakama-match-device-a", "bmx_match_a", True), "first authentication did not start")
AssertTrue(second.AuthenticateDevice("blitzmax-nakama-match-device-b", "bmx_match_b", True), "second authentication did not start")

Local deadline:Int = MilliSecs() + 10000
While (first.AuthenticationState() = NAKAMA_AUTHENTICATION_PENDING Or second.AuthenticationState() = NAKAMA_AUTHENTICATION_PENDING) And MilliSecs() < deadline
	PumpPair(first, second)
Wend
AssertTrue(first.AuthenticationState() = NAKAMA_AUTHENTICATION_SUCCEEDED, "first authentication failed: " + first.ErrorMessage())
AssertTrue(second.AuthenticationState() = NAKAMA_AUTHENTICATION_SUCCEEDED, "second authentication failed: " + second.ErrorMessage())

AssertTrue(first.ConnectSocket(), "first socket connection did not start")
AssertTrue(second.ConnectSocket(), "second socket connection did not start")
deadline = MilliSecs() + 10000
While (first.SocketState() = NAKAMA_SOCKET_CONNECTING Or second.SocketState() = NAKAMA_SOCKET_CONNECTING) And MilliSecs() < deadline
	PumpPair(first, second)
Wend
AssertTrue(first.SocketState() = NAKAMA_SOCKET_CONNECTED, "first socket failed: " + first.ErrorMessage())
AssertTrue(second.SocketState() = NAKAMA_SOCKET_CONNECTED, "second socket failed: " + second.ErrorMessage())

AssertTrue(first.AddMatchmaker(2, 2), "first matchmaking did not start")
AssertTrue(second.AddMatchmaker(2, 2), "second matchmaking did not start")
deadline = MilliSecs() + 15000
While (first.MatchState() <> NAKAMA_MATCH_JOINED Or second.MatchState() <> NAKAMA_MATCH_JOINED) And MilliSecs() < deadline
	PumpPair(first, second)
	If first.MatchState() = NAKAMA_MATCH_FAILED Then Exit
	If second.MatchState() = NAKAMA_MATCH_FAILED Then Exit
Wend
AssertTrue(first.MatchState() = NAKAMA_MATCH_JOINED, "first match join failed: " + first.ErrorMessage())
AssertTrue(second.MatchState() = NAKAMA_MATCH_JOINED, "second match join failed: " + second.ErrorMessage())
AssertTrue(first.MatchId() = second.MatchId(), "clients joined different matches")

AssertTrue(first.SendMatchData(42, "hello from BlitzMax"), "match data send failed")
deadline = MilliSecs() + 5000
While second.ReceivedCount() = 0 And MilliSecs() < deadline
	PumpPair(first, second)
Wend
AssertTrue(second.ReceivedCount() > 0, "second client received no match data")
AssertTrue(second.ReceivedOpCode() = 42, "received operation code differs")
AssertTrue(second.ReceivedData() = "hello from BlitzMax", "received payload differs")
AssertTrue(second.ReceivedSenderId() = first.UserId(), "received sender differs")

Print "Nakama two-client realtime matchmaking and data exchange passed (" + first.MatchId() + ")"
first.Free()
second.Free()

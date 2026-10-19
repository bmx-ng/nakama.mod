SuperStrict

Framework Nakama.SDK
Import BRL.StandardIO

Function AssertTrue(condition:Int, message:String)
	If Not condition Then
		Throw message
	End If
End Function

Local client:TNakamaClient = TNakamaClient.Create("127.0.0.1", 7350, "defaultkey", False, 5000)
AssertTrue(client <> Null, "client creation failed")
AssertTrue(client.AuthenticateDevice("blitzmax-nakama-integration-device", "bmx_integration", True), ..
	"device authentication did not start")

Local deadline:Int = MilliSecs() + 10000
While client.AuthenticationState() = NAKAMA_AUTHENTICATION_PENDING And MilliSecs() < deadline
	client.Tick()
	Delay 10
Wend

AssertTrue(client.AuthenticationState() = NAKAMA_AUTHENTICATION_SUCCEEDED, ..
	"device authentication failed (" + client.ErrorCode() + "): " + client.ErrorMessage())
AssertTrue(client.UserId().Length > 0, "authenticated session has no user id")
AssertTrue(client.Username().Length > 0, "authenticated session has no username")

Print "Nakama live device authentication passed for " + client.Username() + " (" + client.UserId() + ")"
client.Free()

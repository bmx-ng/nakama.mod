SuperStrict

Framework Nakama.SDK
Import BRL.StandardIO

Function AssertTrue(condition:Int, message:String)
	If Not condition Then
		Throw message
	End If
End Function

Local version:String = NakamaSdkVersion()
AssertTrue(version.StartsWith("2.9.0"), "unexpected Nakama SDK version: " + version)

Local client:TNakamaClient = TNakamaClient.Create()
AssertTrue(client <> Null, "client creation failed")
client.Tick()
client.Free()
client.Free()

Print "Nakama.SDK foundational tests passed"

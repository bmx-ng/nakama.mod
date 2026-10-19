SuperStrict

Rem
bbdoc: Native Nakama C++ client SDK.
End Rem
Module Nakama.SDK

ModuleInfo "Version: 0.1.0"
ModuleInfo "License: zlib/libpng"
ModuleInfo "Copyright: Nakama C++ SDK - Heroic Labs"
ModuleInfo "Copyright: Wrapper - 2026 Bruce A Henderson"
ModuleInfo "CPP_OPTS: -std=c++17"
ModuleInfo "CPP_OPTS: -fexceptions"
ModuleInfo "CPP_OPTS: -DWITH_HTTP_CURL -DWITH_WS_WSLAY -DCFG_WSLAY_CURL_IO"
ModuleInfo "CPP_OPTS: -DNAKAMA_STATIC_LIBRARY -DCURL_STATICLIB"
ModuleInfo "CPP_OPTS: -DRAPIDJSON_HAS_STDSTRING=1 -DRAPIDJSON_HAS_CXX11_RVALUE_REFS=1"
ModuleInfo "CPP_OPTS: -I%PWD%/vendor/nakama-cpp/include"
ModuleInfo "CPP_OPTS: -I%PWD%/vendor/nakama-cpp/core/common"
ModuleInfo "CPP_OPTS: -I%PWD%/vendor/nakama-cpp/impl/httpCurl"
ModuleInfo "CPP_OPTS: -I%PWD%/vendor/nakama-cpp/impl/wsWslay"
ModuleInfo "CPP_OPTS: -I%PWD%/vendor/rapidjson/include"
ModuleInfo "CPP_OPTS: -I%PWD%/vendor/wslay/lib"
ModuleInfo "CPP_OPTS: -I%PWD%/vendor/wslay/lib/includes"
ModuleInfo "CPP_OPTS: -I%PWD%/generated"
ModuleInfo "CPP_OPTS: -I%PWD%/../protobuf.mod/vendor/protobuf"
ModuleInfo "CPP_OPTS: -I%PWD%/../../net.mod/libcurl.mod/libcurl/include"
ModuleInfo "CC_OPTS: -DHAVE_CONFIG_H"
ModuleInfo "CC_OPTS: -I%PWD%/vendor/wslay/lib"
ModuleInfo "CC_OPTS: -I%PWD%/vendor/wslay/lib/includes"

Import Nakama.Protobuf
Import Net.libcurl

?win32
Import "-lws2_32"
?

Import "source.bmx"

Extern
	Function bmx_nakama_sdk_version:Byte Ptr()
	Function bmx_nakama_client_create:Byte Ptr(host:Byte Ptr, port:Int, serverKey:Byte Ptr, useSSL:Int, timeoutMs:Int)
	Function bmx_nakama_client_destroy(handle:Byte Ptr)
	Function bmx_nakama_client_tick(handle:Byte Ptr)
	Function bmx_nakama_client_authenticate_device:Int(handle:Byte Ptr, deviceId:Byte Ptr, username:Byte Ptr, create:Int)
	Function bmx_nakama_client_authentication_state:Int(handle:Byte Ptr)
	Function bmx_nakama_client_error_code:Int(handle:Byte Ptr)
	Function bmx_nakama_client_error_message:Byte Ptr(handle:Byte Ptr)
	Function bmx_nakama_client_user_id:Byte Ptr(handle:Byte Ptr)
	Function bmx_nakama_client_username:Byte Ptr(handle:Byte Ptr)
	Function bmx_nakama_client_session_created:Int(handle:Byte Ptr)
	Function bmx_nakama_client_socket_connect:Int(handle:Byte Ptr)
	Function bmx_nakama_client_socket_state:Int(handle:Byte Ptr)
	Function bmx_nakama_client_add_matchmaker:Int(handle:Byte Ptr, minCount:Int, maxCount:Int, query:Byte Ptr, propertyKeys:Byte Ptr Ptr, propertyValues:Byte Ptr Ptr, propertyCount:Int)
	Function bmx_nakama_client_match_state:Int(handle:Byte Ptr)
	Function bmx_nakama_client_match_id:Byte Ptr(handle:Byte Ptr)
	Function bmx_nakama_client_matchmaker_ticket:Byte Ptr(handle:Byte Ptr)
	Function bmx_nakama_client_send_match_data:Int(handle:Byte Ptr, opCode:Long, data:Byte Ptr)
	Function bmx_nakama_client_received_count:Int(handle:Byte Ptr)
	Function bmx_nakama_client_received_opcode:Long(handle:Byte Ptr)
	Function bmx_nakama_client_received_data:Byte Ptr(handle:Byte Ptr)
	Function bmx_nakama_client_received_sender_id:Byte Ptr(handle:Byte Ptr)
	Function bmx_nakama_client_cancel_matchmaker:Int(handle:Byte Ptr)
	Function bmx_nakama_client_match_player_count:Int(handle:Byte Ptr)
	Function bmx_nakama_client_match_self_session_id:Byte Ptr(handle:Byte Ptr)
	Function bmx_nakama_client_match_player_user_id:Byte Ptr(handle:Byte Ptr, index:Int)
	Function bmx_nakama_client_match_player_session_id:Byte Ptr(handle:Byte Ptr, index:Int)
	Function bmx_nakama_client_match_player_username:Byte Ptr(handle:Byte Ptr, index:Int)
	Function bmx_nakama_client_send_match_data_bytes:Int(handle:Byte Ptr, opCode:Long, data:Byte Ptr, length:Size_T, targetSessionId:Byte Ptr)
	Function bmx_nakama_client_leave_match:Int(handle:Byte Ptr)
	Function bmx_nakama_client_socket_disconnect(handle:Byte Ptr)
	Function bmx_nakama_client_poll_event:Int(handle:Byte Ptr)
	Function bmx_nakama_client_event_kind:Int(handle:Byte Ptr)
	Function bmx_nakama_client_event_code:Int(handle:Byte Ptr)
	Function bmx_nakama_client_event_opcode:Long(handle:Byte Ptr)
	Function bmx_nakama_client_event_subject:Byte Ptr(handle:Byte Ptr)
	Function bmx_nakama_client_event_user_id:Byte Ptr(handle:Byte Ptr)
	Function bmx_nakama_client_event_session_id:Byte Ptr(handle:Byte Ptr)
	Function bmx_nakama_client_event_username:Byte Ptr(handle:Byte Ptr)
	Function bmx_nakama_client_event_data:Byte Ptr(handle:Byte Ptr)
	Function bmx_nakama_client_event_data_length:Size_T(handle:Byte Ptr)
	Function bmx_nakama_client_dropped_events:ULong(handle:Byte Ptr)
End Extern

Const NAKAMA_AUTHENTICATION_IDLE:Int = 0
Const NAKAMA_AUTHENTICATION_PENDING:Int = 1
Const NAKAMA_AUTHENTICATION_SUCCEEDED:Int = 2
Const NAKAMA_AUTHENTICATION_FAILED:Int = 3

Const NAKAMA_SOCKET_IDLE:Int = 0
Const NAKAMA_SOCKET_CONNECTING:Int = 1
Const NAKAMA_SOCKET_CONNECTED:Int = 2
Const NAKAMA_SOCKET_DISCONNECTED:Int = 3
Const NAKAMA_SOCKET_FAILED:Int = 4

Const NAKAMA_MATCH_IDLE:Int = 0
Const NAKAMA_MATCH_SEARCHING:Int = 1
Const NAKAMA_MATCH_JOINING:Int = 2
Const NAKAMA_MATCH_JOINED:Int = 3
Const NAKAMA_MATCH_FAILED:Int = 4
Const NAKAMA_MATCH_CANCELLED:Int = 5

Enum ENakamaEventKind:Int
	None
	SocketConnected
	SocketDisconnected
	Error
	MatchmakerTicket
	MatchmakerMatched
	MatchJoined
	MatchPresenceJoined
	MatchPresenceLeft
	MatchData
	MatchmakingCancelled
	MatchLeft
End Enum

Struct SNakamaMatchPlayer
	Field userId:String
	Field sessionId:String
	Field username:String
End Struct

Struct SNakamaEvent
	Field kind:ENakamaEventKind
	Field code:Int
	Field opCode:Long
	Field subject:String
	Field userId:String
	Field sessionId:String
	Field username:String
	Field data:Byte[]
End Struct

Function NakamaSdkVersion:String()
	Return String.FromUTF8String(bmx_nakama_sdk_version())
End Function

Type TNakamaClient
	Field handle:Byte Ptr

	Function Create:TNakamaClient(host:String = "127.0.0.1", port:Int = 7350, serverKey:String = "defaultkey", useSSL:Int = False, timeoutMs:Int = 10000)
		Local hostUtf8:Byte Ptr = host.ToUTF8String()
		Local keyUtf8:Byte Ptr = serverKey.ToUTF8String()
		Local nativeHandle:Byte Ptr = bmx_nakama_client_create(hostUtf8, port, keyUtf8, useSSL, timeoutMs)
		MemFree(hostUtf8)
		MemFree(keyUtf8)

		If Not nativeHandle Then
			Return Null
		End If

		Local client:TNakamaClient = New TNakamaClient
		client.handle = nativeHandle
		Return client
	End Function

	Method Tick()
		If handle Then
			bmx_nakama_client_tick(handle)
		End If
	End Method

	Method AuthenticateDevice:Int(deviceId:String, username:String = "", create:Int = True)
		If Not handle Or Not deviceId Then
			Return False
		End If

		Local deviceUtf8:Byte Ptr = deviceId.ToUTF8String()
		Local usernameUtf8:Byte Ptr = username.ToUTF8String()
		Local started:Int = bmx_nakama_client_authenticate_device(handle, deviceUtf8, usernameUtf8, create)
		MemFree(deviceUtf8)
		MemFree(usernameUtf8)
		Return started
	End Method

	Method AuthenticationState:Int()
		If Not handle Then Return NAKAMA_AUTHENTICATION_FAILED
		Return bmx_nakama_client_authentication_state(handle)
	End Method

	Method ErrorCode:Int()
		If Not handle Then Return -2
		Return bmx_nakama_client_error_code(handle)
	End Method

	Method ErrorMessage:String()
		If Not handle Then Return "Invalid Nakama client."
		Return String.FromUTF8String(bmx_nakama_client_error_message(handle))
	End Method

	Method UserId:String()
		If Not handle Then Return ""
		Return String.FromUTF8String(bmx_nakama_client_user_id(handle))
	End Method

	Method Username:String()
		If Not handle Then Return ""
		Return String.FromUTF8String(bmx_nakama_client_username(handle))
	End Method

	Method SessionCreated:Int()
		If Not handle Then Return False
		Return bmx_nakama_client_session_created(handle)
	End Method

	Method ConnectSocket:Int()
		If Not handle Then Return False
		Return bmx_nakama_client_socket_connect(handle)
	End Method

	Method SocketState:Int()
		If Not handle Then Return NAKAMA_SOCKET_FAILED
		Return bmx_nakama_client_socket_state(handle)
	End Method

	Method AddMatchmaker:Int(minCount:Int = 2, maxCount:Int = 2)
		Return AddMatchmakerProperties(minCount, maxCount, "", New String[0], New String[0])
	End Method

	Method AddMatchmakerProperties:Int(minCount:Int, maxCount:Int, query:String, propertyKeys:String[], propertyValues:String[])
		If Not handle Then Return False
		Local propertyCount:Int
		If propertyKeys Then propertyCount = propertyKeys.Length
		Local valueCount:Int
		If propertyValues Then valueCount = propertyValues.Length
		If propertyCount <> valueCount Then Return False
		Local queryUtf8:Byte Ptr = query.ToUTF8String()
		Local keyPointers:Byte Ptr[] = New Byte Ptr[propertyCount]
		Local valuePointers:Byte Ptr[] = New Byte Ptr[propertyCount]
		For Local index:Int = 0 Until propertyCount
			keyPointers[index] = propertyKeys[index].ToUTF8String()
			valuePointers[index] = propertyValues[index].ToUTF8String()
		Next
		Local keysAddress:Byte Ptr Ptr
		Local valuesAddress:Byte Ptr Ptr
		If propertyCount Then
			keysAddress = Byte Ptr Ptr(keyPointers)
			valuesAddress = Byte Ptr Ptr(valuePointers)
		End If
		Local started:Int = bmx_nakama_client_add_matchmaker(handle, minCount, maxCount, queryUtf8, keysAddress, valuesAddress, propertyCount)
		For Local index:Int = 0 Until propertyCount
			MemFree(keyPointers[index])
			MemFree(valuePointers[index])
		Next
		MemFree(queryUtf8)
		Return started
	End Method

	Method CancelMatchmaker:Int()
		If Not handle Then Return False
		Return bmx_nakama_client_cancel_matchmaker(handle)
	End Method

	Method MatchState:Int()
		If Not handle Then Return NAKAMA_MATCH_FAILED
		Return bmx_nakama_client_match_state(handle)
	End Method

	Method MatchId:String()
		If Not handle Then Return ""
		Return String.FromUTF8String(bmx_nakama_client_match_id(handle))
	End Method

	Method MatchmakerTicket:String()
		If Not handle Then Return ""
		Return String.FromUTF8String(bmx_nakama_client_matchmaker_ticket(handle))
	End Method

	Method SendMatchData:Int(opCode:Long, data:String)
		If Not handle Then Return False
		Local dataUtf8:Byte Ptr = data.ToUTF8String()
		Local sent:Int = bmx_nakama_client_send_match_data(handle, opCode, dataUtf8)
		MemFree(dataUtf8)
		Return sent
	End Method

	Method ReceivedCount:Int()
		If Not handle Then Return 0
		Return bmx_nakama_client_received_count(handle)
	End Method

	Method ReceivedOpCode:Long()
		If Not handle Then Return 0
		Return bmx_nakama_client_received_opcode(handle)
	End Method

	Method ReceivedData:String()
		If Not handle Then Return ""
		Return String.FromUTF8String(bmx_nakama_client_received_data(handle))
	End Method

	Method ReceivedSenderId:String()
		If Not handle Then Return ""
		Return String.FromUTF8String(bmx_nakama_client_received_sender_id(handle))
	End Method

	Method MatchPlayerCount:Int()
		If Not handle Then Return 0
		Return bmx_nakama_client_match_player_count(handle)
	End Method

	Method MatchSelfSessionId:String()
		If Not handle Then Return ""
		Return String.FromUTF8String(bmx_nakama_client_match_self_session_id(handle))
	End Method

	Method MatchPlayer:SNakamaMatchPlayer(index:Int)
		Local result:SNakamaMatchPlayer
		If Not handle Or index < 0 Or index >= MatchPlayerCount() Then Return result
		result.userId = String.FromUTF8String(bmx_nakama_client_match_player_user_id(handle, index))
		result.sessionId = String.FromUTF8String(bmx_nakama_client_match_player_session_id(handle, index))
		result.username = String.FromUTF8String(bmx_nakama_client_match_player_username(handle, index))
		Return result
	End Method

	Method SendMatchDataBytes:Int(opCode:Long, data:Byte Ptr, length:Size_T, targetSessionId:String = "")
		If Not handle Or (length > 0 And Not data) Then Return False
		Local targetUtf8:Byte Ptr = targetSessionId.ToUTF8String()
		Local sent:Int = bmx_nakama_client_send_match_data_bytes(handle, opCode, data, length, targetUtf8)
		MemFree(targetUtf8)
		Return sent
	End Method

	Method LeaveMatch:Int()
		If Not handle Then Return False
		Return bmx_nakama_client_leave_match(handle)
	End Method

	Method DisconnectSocket()
		If handle Then bmx_nakama_client_socket_disconnect(handle)
	End Method

	Method Poll:Int(event:SNakamaEvent Var)
		If Not handle Or Not bmx_nakama_client_poll_event(handle) Then
			event = New SNakamaEvent
			Return False
		End If
		event = New SNakamaEvent
		event.kind = ENakamaEventKind(bmx_nakama_client_event_kind(handle))
		event.code = bmx_nakama_client_event_code(handle)
		event.opCode = bmx_nakama_client_event_opcode(handle)
		event.subject = String.FromUTF8String(bmx_nakama_client_event_subject(handle))
		event.userId = String.FromUTF8String(bmx_nakama_client_event_user_id(handle))
		event.sessionId = String.FromUTF8String(bmx_nakama_client_event_session_id(handle))
		event.username = String.FromUTF8String(bmx_nakama_client_event_username(handle))
		Local length:Size_T = bmx_nakama_client_event_data_length(handle)
		If length Then
			event.data = New Byte[Int(length)]
			MemCopy(Byte Ptr(event.data), bmx_nakama_client_event_data(handle), length)
		Else
			event.data = New Byte[0]
		End If
		Return True
	End Method

	Method DroppedEvents:ULong()
		If Not handle Then Return 0
		Return bmx_nakama_client_dropped_events(handle)
	End Method

	Method Free()
		If handle Then
			bmx_nakama_client_destroy(handle)
			handle = Null
		End If
	End Method

	Method Delete()
		Free()
	End Method
End Type

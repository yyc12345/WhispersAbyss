@echo off
protoc -I=./ --cpp_out=../WhispersAbyss/protobuf_src --csharp_out=../ShadowWalker/Protobuf  cmmo.proto
pause
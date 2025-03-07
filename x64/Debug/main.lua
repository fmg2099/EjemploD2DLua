local simpledraw = require "SimpleDraw"

print("hola mundo desde main.lua")

function Draw(dt)
	--print("draw desde lua" .. dt)
	SimpleDraw.Clear(20,20,20)
	SimpleDraw.SetBrushColor(128,0,0)
	simpledraw.DrawCircle(100,50,28)
	SimpleDraw.SetBrushColor(0,128,0)
	simpledraw.DrawCircle(150,100,28)
	SimpleDraw.SetBrushColor(0,0,128)
	simpledraw.DrawCircle(200,150,28)
	SimpleDraw.SetBrushColor(255,255,0)
	SimpleDraw.DrawLine(100,100,500,100)
	SimpleDraw.DrawLine(100,500,500,500)
	SimpleDraw.SetBrushColor(200,200,255)
	SimpleDraw.DrawRect(300,300,100,100)
end

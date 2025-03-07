local simpledraw = require "SimpleDraw"

print("hola mundo desde main.lua")

function Draw (dt)
	--print("draw desde lua" .. dt)
	SimpleDraw.Clear(255,250,250)
	simpledraw.DrawCircle(500,500,28)
end

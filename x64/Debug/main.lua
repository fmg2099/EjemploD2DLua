print("hola mundo desde luAAa")

function Draw(dt)
	print("draw desde lua" .. dt)
end

function functionFromC(dt)
	print("enviado desde C: " .. dt)
	testlib.log("la magnitud es " .. testlib.vec2mag(4,3))

end

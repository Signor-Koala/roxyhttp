Overrides = {
	["lua_test"] = "Hello_func",
	["lua/test2"] = "Hello_func2",
}

function Hello_func(req)
	return [[<!DOCTYPE html>
<html lang=\"en\">
<body>

<h1>My Lua Heading</h1>
<p>My first paragraph.</p>

</body>
</html>]]
end

function Hello_func2(req)
	return [[<!DOCTYPE html>
<html lang=\"en\">
<body>

<h1>My Second Lua Heading</h1>
<p>My first paragraph.</p>

</body>
</html>]]
end

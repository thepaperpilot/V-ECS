return {
	name = "Incremental Game Jam 2020",
	systems = {
		hourglass = "gamejam/hourglass.lua",
		camera = "systems/camera.lua",
		stone = "gamejam/stone.lua",
		fps = "systems/fps.lua",
		debug = "systems/debug.lua",
		loading = "systems/loading.lua"
	},
	renderers = {
		sprite = "renderers/sprite.lua",
		stone = "renderers/stone.lua",
		imgui = "renderers/imgui.lua"
	}
}

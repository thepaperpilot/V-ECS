return {
	name = "Voxel Test",
	systems = {
		terrain = "systems/terrain.lua",
		noclip = "systems/noclip.lua",
		camera = "systems/camera.lua",
		fps = "systems/fps.lua",
		hotbar = "systems/hotbar.lua",
		debug = "systems/debug.lua"
	},
	renderers = {
		skybox = "renderers/skybox.lua",
		gundam = "renderers/example_gundam.lua",
		voxel = "renderers/voxel.lua",
		imgui = "renderers/imgui.lua"
	}
}

# Animation Guide for Pepsi Man 3D

## Current Status
Your FBX character (`objects/fbx_Clean/fbx Clean.fbx`) is a **static T-pose** — 0 animation channels, 480 bones, 37 textures. It renders fine but does not move. To get a running/jumping character you need an animated FBX.

---

## Step 1 — Get an Animated FBX from Mixamo

**Site:** https://www.mixamo.com  
**Cost:** Free (requires a free Adobe account)

### What to do:
1. Create a free Adobe account at https://account.adobe.com
2. Go to https://www.mixamo.com and sign in
3. Click **Upload Character** (top right) and upload your FBX file:
   `objects/fbx_Clean/fbx Clean.fbx`
4. Wait for Mixamo to rig it (auto-rigging takes ~1 minute)
5. Browse animations — search for:
   - **"Running"** → for the default running state
   - **"Jump"** → for the jump action
   - **"Idle"** → for standing still
   - **"Left Strafe"** / **"Right Strafe"** → for moving left/right
6. For each animation:
   - Click the animation to preview it on your character
   - Click **Download**
   - Settings: **Format = FBX Binary**, **Skin = With Skin**, **Frames = 30**
   - Save each file with a clear name, e.g.:
     - `objects/animations/run.fbx`
     - `objects/animations/jump.fbx`
     - `objects/animations/idle.fbx`

---

## Step 2 — Alternative Free Animation Sites

If Mixamo does not work well with your character:

| Site | URL | Notes |
|------|-----|-------|
| Mixamo | https://www.mixamo.com | Best auto-rigging, free |
| Sketchfab | https://sketchfab.com | Search for animated FBX, some free |
| Turbosquid | https://www.turbosquid.com | Filter by Free + Animated |
| CGTrader | https://www.cgtrader.com | Filter by Free + Rigged |
| Kenney.nl | https://kenney.nl/assets | Free game assets, simple characters |

---

## Step 3 — What to Tell the Developer (me)

Once you have the animated FBX files downloaded, tell me:
- Where you saved them (e.g. `objects/animations/run.fbx`)
- Which animation is for running, jumping, idle

I will then:
1. Add CPU skinning to `AssimpModel` to process bone weights
2. Add an animation player that advances keyframes each frame
3. Wire run/jump/idle to the keyboard controls (arrow keys + space)

---

## Step 4 — File Naming Convention (recommended)

```
objects/
  animations/
    run.fbx        ← looping run cycle
    jump.fbx       ← jump (play once)
    idle.fbx       ← looping idle/stand
    left.fbx       ← optional: strafe left
    right.fbx      ← optional: strafe right
```

---

## Notes

- Download each animation **separately** from Mixamo (one animation per file)
- Always choose **"With Skin"** so the mesh comes with the animation
- FBX Binary is smaller and faster to load than FBX ASCII
- The character does not need to be re-uploaded each time — once rigged on Mixamo, it stays in your account

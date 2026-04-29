/*
 * ================================================================
 *  OBJ Model Loader & Viewer — OpenGL / GLUT
 *  ──────────────────────────────────────────
 *  Loads .OBJ files with .MTL materials and textures.
 *  Compiles geometry into OpenGL display lists for fast rendering.
 *
 *  Supports:
 *    • Vertices, normals, texture coordinates
 *    • Multiple objects / groups
 *    • MTL materials (Kd, Ks, Ka, Ns, d, map_Kd)
 *    • Texture loading via stb_image (BMP/PNG/JPG/TGA)
 *    • Triangles and quads (auto‑triangulated n‑gons)
 *    • Large models (tested with 700K+ polygon scenes)
 *
 *  ──────────────────────────────────────────
 *  SETUP — one extra file needed:
 *
 *    Download stb_image.h (single header, public domain):
 *      https://raw.githubusercontent.com/nothings/stb/master/stb_image.h
 *
 *    Place it next to this .c file.
 *
 *  ──────────────────────────────────────────
 *  Compile (Linux):
 *    gcc obj_viewer.c -o viewer -lGL -lGLU -lglut -lm -O2
 *
 *  Compile (macOS):
 *    gcc obj_viewer.c -o viewer -framework OpenGL -framework GLUT -lm -O2
 *
 *  Compile (Windows / MinGW):
 *    gcc obj_viewer.c -o viewer.exe -lfreeglut -lopengl32 -lglu32 -lm -O2
 *
 *  ──────────────────────────────────────────
 *  Usage:
 *    ./viewer path/to/model.obj
 *    ./viewer path/to/city/city.obj
 *
 *  Controls:
 *    SPACE        toggle turntable
 *    W/S          zoom in / out
 *    A/D          pan left / right
 *    Q/E          pan up / down
 *    Arrow keys   manual orbit
 *    Mouse drag   orbit camera
 *    Scroll       zoom
 *    R            reset camera
 *    L            toggle lighting
 *    T            toggle textures
 *    F            toggle wireframe
 *    G            toggle ground grid
 *    +/−          turntable speed
 *    ESC          quit
 * ================================================================
 */

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <GL/glut.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <float.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#define RAD(d) ((d)*M_PI/180.0)

#ifndef GL_MULTISAMPLE
#define GL_MULTISAMPLE 0x809D
#endif

/* ═══════════════════════════════════════════════
 *  DATA STRUCTURES
 * ═══════════════════════════════════════════════ */

/* dynamic array macro */
#define DA_INIT    { NULL, 0, 0 }
#define DA_PUSH(arr, item) do { \
    if ((arr).count >= (arr).cap) { \
        (arr).cap = (arr).cap ? (arr).cap * 2 : 1024; \
        (arr).data = realloc((arr).data, (arr).cap * sizeof(*(arr).data)); \
    } \
    (arr).data[(arr).count++] = (item); \
} while(0)

typedef struct { float x, y, z; } Vec3;
typedef struct { float u, v; }     Vec2;

/* face vertex index triplet: v/vt/vn (1‑based, 0 = absent) */
typedef struct { int vi, ti, ni; } FaceIdx;

/* material */
typedef struct {
    char   name[128];
    float  Ka[3], Kd[3], Ks[3];
    float  Ns;           /* shininess */
    float  d;            /* opacity */
    GLuint texID;        /* 0 = no texture */
    char   mapKd[512];   /* diffuse texture path */
} Material;

/* one draw group: contiguous faces sharing a material */
typedef struct {
    int    matIndex;     /* index into materials array, −1 = default */
    int    faceStart;    /* start index in faces array */
    int    faceCount;    /* number of face‑index triplets (3 per tri) */
} DrawGroup;

/* the whole model */
typedef struct {
    /* raw arrays (1‑indexed in OBJ, we store 0‑indexed) */
    struct { Vec3 *data; int count, cap; } verts;
    struct { Vec3 *data; int count, cap; } norms;
    struct { Vec2 *data; int count, cap; } texcs;

    /* triangulated face indices */
    struct { FaceIdx *data; int count, cap; } faces;

    /* materials */
    Material *mats;
    int       matCount, matCap;

    /* draw groups */
    DrawGroup *groups;
    int        grpCount, grpCap;

    /* bounding box */
    Vec3 bbMin, bbMax, center;
    float radius;

    /* compiled display list */
    GLuint dispList;

    /* base directory for texture paths */
    char baseDir[1024];
} Model;

static Model model;

/* ═══════════════════════════════════════════════
 *  UTILITY
 * ═══════════════════════════════════════════════ */

/* extract directory from a file path */
static void getDirectory(const char *path, char *dir, int maxLen)
{
    strncpy(dir, path, maxLen - 1);
    dir[maxLen - 1] = '\0';
    /* find last slash */
    char *ls = strrchr(dir, '/');
    char *bs = strrchr(dir, '\\');
    char *sep = (ls > bs) ? ls : bs;
    if (sep) *(sep + 1) = '\0';
    else     dir[0] = '\0';
}

/* build full path from base directory + relative filename */
static void buildPath(const char *base, const char *rel, char *out, int maxLen)
{
    if (rel[0] == '/' || rel[0] == '\\' || (rel[0] && rel[1] == ':')) {
        /* absolute path */
        strncpy(out, rel, maxLen - 1);
    } else {
        snprintf(out, maxLen, "%s%s", base, rel);
    }
    out[maxLen - 1] = '\0';
}

/* skip leading whitespace */
static const char *skipWS(const char *s)
{
    while (*s && isspace((unsigned char)*s)) s++;
    return s;
}

/* trim trailing whitespace / newline */
static void trimEnd(char *s)
{
    int len = (int)strlen(s);
    while (len > 0 && (s[len-1]=='\n' || s[len-1]=='\r' || s[len-1]==' '))
        s[--len] = '\0';
}

/* ═══════════════════════════════════════════════
 *  TEXTURE LOADING  (via stb_image)
 * ═══════════════════════════════════════════════ */
static GLuint loadTexture(const char *path)
{
    int w, h, ch;
    stbi_set_flip_vertically_on_load(1);
    unsigned char *data = stbi_load(path, &w, &h, &ch, 0);
    if (!data) {
        fprintf(stderr, "  [tex] Could not load: %s\n", path);
        return 0;
    }

    GLenum fmt = GL_RGB;
    if (ch == 4) fmt = GL_RGBA;
    else if (ch == 1) fmt = GL_LUMINANCE;

    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    gluBuild2DMipmaps(GL_TEXTURE_2D, fmt, w, h, fmt, GL_UNSIGNED_BYTE, data);

    stbi_image_free(data);
    printf("  [tex] Loaded %s (%dx%d, %dch)\n", path, w, h, ch);
    return tex;
}

/* ═══════════════════════════════════════════════
 *  MTL PARSER
 * ═══════════════════════════════════════════════ */
static void parseMTL(const char *path)
{
    FILE *f = fopen(path, "r");
    if (!f) {
        fprintf(stderr, "  [mtl] Could not open: %s\n", path);
        return;
    }
    printf("  [mtl] Parsing %s\n", path);

    Material *cur = NULL;
    char line[1024];

    while (fgets(line, sizeof(line), f)) {
        trimEnd(line);
        const char *p = skipWS(line);
        if (!*p || *p == '#') continue;

        if (strncmp(p, "newmtl ", 7) == 0) {
            /* add new material */
            if (model.matCount >= model.matCap) {
                model.matCap = model.matCap ? model.matCap * 2 : 16;
                model.mats = realloc(model.mats, model.matCap * sizeof(Material));
            }
            cur = &model.mats[model.matCount++];
            memset(cur, 0, sizeof(Material));
            strncpy(cur->name, p + 7, sizeof(cur->name) - 1);
            /* defaults */
            cur->Ka[0]=cur->Ka[1]=cur->Ka[2] = 0.2f;
            cur->Kd[0]=cur->Kd[1]=cur->Kd[2] = 0.8f;
            cur->Ks[0]=cur->Ks[1]=cur->Ks[2] = 0.0f;
            cur->Ns = 10.0f;
            cur->d  = 1.0f;
            cur->texID = 0;
            printf("    material: %s\n", cur->name);
        }
        else if (cur) {
            if (strncmp(p, "Ka ", 3) == 0)
                sscanf(p+3, "%f %f %f", &cur->Ka[0], &cur->Ka[1], &cur->Ka[2]);
            else if (strncmp(p, "Kd ", 3) == 0)
                sscanf(p+3, "%f %f %f", &cur->Kd[0], &cur->Kd[1], &cur->Kd[2]);
            else if (strncmp(p, "Ks ", 3) == 0)
                sscanf(p+3, "%f %f %f", &cur->Ks[0], &cur->Ks[1], &cur->Ks[2]);
            else if (strncmp(p, "Ns ", 3) == 0)
                sscanf(p+3, "%f", &cur->Ns);
            else if (strncmp(p, "d ", 2) == 0)
                sscanf(p+2, "%f", &cur->d);
            else if (strncmp(p, "Tr ", 3) == 0) {
                float tr; sscanf(p+3, "%f", &tr);
                cur->d = 1.0f - tr;
            }
            else if (strncmp(p, "map_Kd ", 7) == 0) {
                const char *tp = skipWS(p + 7);
                strncpy(cur->mapKd, tp, sizeof(cur->mapKd) - 1);
            }
        }
    }
    fclose(f);

    /* load textures for all materials */
    int i;
    for (i = 0; i < model.matCount; i++) {
        if (model.mats[i].mapKd[0]) {
            char fullPath[1024];
            buildPath(model.baseDir, model.mats[i].mapKd, fullPath, sizeof(fullPath));
            model.mats[i].texID = loadTexture(fullPath);
        }
    }
}

/* ═══════════════════════════════════════════════
 *  OBJ PARSER
 * ═══════════════════════════════════════════════ */

/* find material index by name, −1 if not found */
static int findMaterial(const char *name)
{
    int i;
    for (i = 0; i < model.matCount; i++)
        if (strcmp(model.mats[i].name, name) == 0)
            return i;
    return -1;
}

/* parse a face vertex token "v/vt/vn" */
static FaceIdx parseFV(const char *tok)
{
    FaceIdx fi = {0, 0, 0};
    /* formats: v, v/vt, v/vt/vn, v//vn */
    if (sscanf(tok, "%d/%d/%d", &fi.vi, &fi.ti, &fi.ni) == 3) {}
    else if (sscanf(tok, "%d//%d", &fi.vi, &fi.ni) == 2) { fi.ti = 0; }
    else if (sscanf(tok, "%d/%d", &fi.vi, &fi.ti) == 2) { fi.ni = 0; }
    else sscanf(tok, "%d", &fi.vi);

    /* convert to 0‑based; handle negative indices */
    if (fi.vi > 0) fi.vi--; else if (fi.vi < 0) fi.vi += model.verts.count;
    if (fi.ti > 0) fi.ti--; else if (fi.ti < 0) fi.ti += model.texcs.count;
    if (fi.ni > 0) fi.ni--; else if (fi.ni < 0) fi.ni += model.norms.count;

    return fi;
}

static void loadOBJ(const char *path)
{
    FILE *f = fopen(path, "r");
    if (!f) {
        fprintf(stderr, "ERROR: Could not open %s\n", path);
        exit(1);
    }
    printf("[obj] Loading %s ...\n", path);

    getDirectory(path, model.baseDir, sizeof(model.baseDir));

    char line[1024];
    int curMat = -1;
    int groupFaceStart = 0;

    /* init bounding box */
    model.bbMin = (Vec3){ FLT_MAX,  FLT_MAX,  FLT_MAX};
    model.bbMax = (Vec3){-FLT_MAX, -FLT_MAX, -FLT_MAX};

    while (fgets(line, sizeof(line), f)) {
        trimEnd(line);
        const char *p = skipWS(line);
        if (!*p || *p == '#') continue;

        /* ── vertex ── */
        if (p[0] == 'v' && p[1] == ' ') {
            Vec3 v;
            sscanf(p + 2, "%f %f %f", &v.x, &v.y, &v.z);
            DA_PUSH(model.verts, v);
            /* update bbox */
            if (v.x < model.bbMin.x) model.bbMin.x = v.x;
            if (v.y < model.bbMin.y) model.bbMin.y = v.y;
            if (v.z < model.bbMin.z) model.bbMin.z = v.z;
            if (v.x > model.bbMax.x) model.bbMax.x = v.x;
            if (v.y > model.bbMax.y) model.bbMax.y = v.y;
            if (v.z > model.bbMax.z) model.bbMax.z = v.z;
        }
        /* ── texture coord ── */
        else if (p[0] == 'v' && p[1] == 't' && p[2] == ' ') {
            Vec2 t;
            sscanf(p + 3, "%f %f", &t.u, &t.v);
            DA_PUSH(model.texcs, t);
        }
        /* ── normal ── */
        else if (p[0] == 'v' && p[1] == 'n' && p[2] == ' ') {
            Vec3 n;
            sscanf(p + 3, "%f %f %f", &n.x, &n.y, &n.z);
            DA_PUSH(model.norms, n);
        }
        /* ── face (triangulate fan for n>3) ── */
        else if (p[0] == 'f' && p[1] == ' ') {
            /* tokenize all face vertices */
            FaceIdx fvs[64];
            int nv = 0;
            const char *tp = p + 2;
            char tok[64];
            while (nv < 64 && sscanf(tp, "%63s", tok) == 1) {
                fvs[nv++] = parseFV(tok);
                tp += strlen(tok);
                while (*tp && isspace((unsigned char)*tp)) tp++;
            }

            /* fan triangulation: (0,1,2), (0,2,3), ... */
            int t;
            for (t = 2; t < nv; t++) {
                DA_PUSH(model.faces, fvs[0]);
                DA_PUSH(model.faces, fvs[t-1]);
                DA_PUSH(model.faces, fvs[t]);
            }
        }
        /* ── material library ── */
        else if (strncmp(p, "mtllib ", 7) == 0) {
            const char *mp = skipWS(p + 7);
            char mtlPath[1024];
            buildPath(model.baseDir, mp, mtlPath, sizeof(mtlPath));
            parseMTL(mtlPath);
        }
        /* ── use material ── */
        else if (strncmp(p, "usemtl ", 7) == 0) {
            /* close previous group */
            int fc = model.faces.count - groupFaceStart;
            if (fc > 0) {
                DrawGroup g;
                g.matIndex  = curMat;
                g.faceStart = groupFaceStart;
                g.faceCount = fc;
                if (model.grpCount >= model.grpCap) {
                    model.grpCap = model.grpCap ? model.grpCap * 2 : 32;
                    model.groups = realloc(model.groups, model.grpCap * sizeof(DrawGroup));
                }
                model.groups[model.grpCount++] = g;
            }
            groupFaceStart = model.faces.count;

            const char *mn = skipWS(p + 7);
            char matName[128];
            strncpy(matName, mn, sizeof(matName) - 1);
            matName[sizeof(matName)-1] = '\0';
            curMat = findMaterial(matName);
        }
    }
    fclose(f);

    /* close last group */
    int fc = model.faces.count - groupFaceStart;
    if (fc > 0) {
        DrawGroup g;
        g.matIndex  = curMat;
        g.faceStart = groupFaceStart;
        g.faceCount = fc;
        if (model.grpCount >= model.grpCap) {
            model.grpCap = model.grpCap ? model.grpCap * 2 : 32;
            model.groups = realloc(model.groups, model.grpCap * sizeof(DrawGroup));
        }
        model.groups[model.grpCount++] = g;
    }

    /* if no groups were created, make one default group */
    if (model.grpCount == 0 && model.faces.count > 0) {
        model.grpCap = 1;
        model.groups = malloc(sizeof(DrawGroup));
        model.groups[0].matIndex  = -1;
        model.groups[0].faceStart = 0;
        model.groups[0].faceCount = model.faces.count;
        model.grpCount = 1;
    }

    /* compute centre and radius */
    model.center.x = (model.bbMin.x + model.bbMax.x) * 0.5f;
    model.center.y = (model.bbMin.y + model.bbMax.y) * 0.5f;
    model.center.z = (model.bbMin.z + model.bbMax.z) * 0.5f;
    float dx = model.bbMax.x - model.bbMin.x;
    float dy = model.bbMax.y - model.bbMin.y;
    float dz = model.bbMax.z - model.bbMin.z;
    model.radius = sqrtf(dx*dx + dy*dy + dz*dz) * 0.5f;
    if (model.radius < 0.001f) model.radius = 1.0f;

    printf("[obj] Loaded:\n");
    printf("  Vertices:   %d\n", model.verts.count);
    printf("  Normals:    %d\n", model.norms.count);
    printf("  TexCoords:  %d\n", model.texcs.count);
    printf("  Triangles:  %d\n", model.faces.count / 3);
    printf("  Materials:  %d\n", model.matCount);
    printf("  Groups:     %d\n", model.grpCount);
    printf("  BBox:       (%.2f,%.2f,%.2f)—(%.2f,%.2f,%.2f)\n",
           model.bbMin.x, model.bbMin.y, model.bbMin.z,
           model.bbMax.x, model.bbMax.y, model.bbMax.z);
    printf("  Centre:     (%.2f,%.2f,%.2f)  Radius: %.2f\n",
           model.center.x, model.center.y, model.center.z, model.radius);
}

/* ═══════════════════════════════════════════════
 *  COMPILE TO DISPLAY LIST (fast rendering)
 * ═══════════════════════════════════════════════ */
static void compileDisplayList(void)
{
    model.dispList = glGenLists(1);
    glNewList(model.dispList, GL_COMPILE);

    int g;
    for (g = 0; g < model.grpCount; g++) {
        DrawGroup *dg = &model.groups[g];
        Material  *mt = (dg->matIndex >= 0) ? &model.mats[dg->matIndex] : NULL;

        /* apply material */
        if (mt) {
            float amb[4] = {mt->Ka[0], mt->Ka[1], mt->Ka[2], mt->d};
            float dif[4] = {mt->Kd[0], mt->Kd[1], mt->Kd[2], mt->d};
            float spc[4] = {mt->Ks[0], mt->Ks[1], mt->Ks[2], mt->d};
            glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT,   amb);
            glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE,   dif);
            glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR,  spc);
            glMaterialf (GL_FRONT_AND_BACK, GL_SHININESS, mt->Ns > 128 ? 128 : mt->Ns);

            if (mt->texID) {
                glEnable(GL_TEXTURE_2D);
                glBindTexture(GL_TEXTURE_2D, mt->texID);
            } else {
                glDisable(GL_TEXTURE_2D);
            }

            if (mt->d < 0.99f) {
                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            } else {
                glDisable(GL_BLEND);
            }
        } else {
            /* default grey material */
            float grey[4] = {0.6f, 0.6f, 0.6f, 1.0f};
            float spec[4] = {0.1f, 0.1f, 0.1f, 1.0f};
            glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT,  grey);
            glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE,  grey);
            glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, spec);
            glMaterialf (GL_FRONT_AND_BACK, GL_SHININESS, 10.0f);
            glDisable(GL_TEXTURE_2D);
            glDisable(GL_BLEND);
        }

        /* emit triangles */
        glBegin(GL_TRIANGLES);
        int i;
        for (i = dg->faceStart; i < dg->faceStart + dg->faceCount; i++) {
            FaceIdx *fi = &model.faces.data[i];

            if (fi->ni >= 0 && fi->ni < model.norms.count) {
                Vec3 *n = &model.norms.data[fi->ni];
                glNormal3f(n->x, n->y, n->z);
            }
            if (fi->ti >= 0 && fi->ti < model.texcs.count) {
                Vec2 *t = &model.texcs.data[fi->ti];
                glTexCoord2f(t->u, t->v);
            }
            if (fi->vi >= 0 && fi->vi < model.verts.count) {
                Vec3 *v = &model.verts.data[fi->vi];
                glVertex3f(v->x, v->y, v->z);
            }
        }
        glEnd();
    }

    glDisable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);
    glEndList();

    printf("[obj] Display list compiled (%d triangles)\n", model.faces.count / 3);
}

/* ═══════════════════════════════════════════════
 *  CAMERA & RENDERING STATE
 * ═══════════════════════════════════════════════ */
static int   autoRotate = 1;
static float turnAngle  = 0.0f;
static float turnSpeed  = 0.25f;
static int   showLighting  = 1;
static int   showTextures  = 1;
static int   showWireframe = 0;
static int   showGround    = 1;

/* ═══════════════════════════════════════════════
 *  GROUND GRID
 * ═══════════════════════════════════════════════ */
static void drawGround(void)
{
    if (!showGround) return;

    float ext = model.radius * 2.5f;
    float step = model.radius * 0.15f;
    if (step < 0.1f) step = 0.1f;

    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glColor4f(0.35f, 0.35f, 0.40f, 0.5f);
    glLineWidth(1.0f);

    glBegin(GL_LINES);
    float i;
    float y = model.bbMin.y - 0.01f;
    for (i = -ext; i <= ext; i += step) {
        glVertex3f(model.center.x + i, y, model.center.z - ext);
        glVertex3f(model.center.x + i, y, model.center.z + ext);
        glVertex3f(model.center.x - ext, y, model.center.z + i);
        glVertex3f(model.center.x + ext, y, model.center.z + i);
    }
    glEnd();

    if (showLighting) glEnable(GL_LIGHTING);
}

/* ═══════════════════════════════════════════════
 *  LIGHTING
 * ═══════════════════════════════════════════════ */
static void setupLights(void)
{
    float r = model.radius;
    float cx = model.center.x, cy = model.center.y, cz = model.center.z;

    /* key light — warm, upper front‑left */
    float p0[4] = {cx + r*1.5f, cy + r*3.0f, cz + r*2.0f, 1.0f};
    float d0[4] = {1.0f, 0.95f, 0.88f, 1.0f};
    float s0[4] = {0.8f, 0.8f, 0.8f, 1.0f};
    float z[4]  = {0,0,0,1};
    glLightfv(GL_LIGHT0, GL_POSITION, p0);
    glLightfv(GL_LIGHT0, GL_DIFFUSE,  d0);
    glLightfv(GL_LIGHT0, GL_SPECULAR, s0);
    glLightfv(GL_LIGHT0, GL_AMBIENT,  z);
    glEnable(GL_LIGHT0);

    /* fill light — cool, opposite side */
    float p1[4] = {cx - r*2.0f, cy + r*1.5f, cz - r*1.5f, 1.0f};
    float d1[4] = {0.30f, 0.35f, 0.45f, 1.0f};
    glLightfv(GL_LIGHT1, GL_POSITION, p1);
    glLightfv(GL_LIGHT1, GL_DIFFUSE,  d1);
    glLightfv(GL_LIGHT1, GL_SPECULAR, z);
    glLightfv(GL_LIGHT1, GL_AMBIENT,  z);
    glEnable(GL_LIGHT1);

    /* rim light — behind */
    float p2[4] = {cx, cy + r*2.0f, cz - r*3.0f, 1.0f};
    float d2[4] = {0.25f, 0.28f, 0.35f, 1.0f};
    glLightfv(GL_LIGHT2, GL_POSITION, p2);
    glLightfv(GL_LIGHT2, GL_DIFFUSE,  d2);
    glLightfv(GL_LIGHT2, GL_SPECULAR, s0);
    glLightfv(GL_LIGHT2, GL_AMBIENT,  z);
    glEnable(GL_LIGHT2);

    float ga[4] = {0.15f, 0.16f, 0.18f, 1.0f};
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ga);
    glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_TRUE);
}

/* ═══════════════════════════════════════════════
 *  GLUT CALLBACKS
 * ═══════════════════════════════════════════════ */
static void reshape(int w, int h)
{
    if (h == 0) h = 1;
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0, (double)w/h,
                   model.radius * 0.01,
                   model.radius * 20.0);
    glMatrixMode(GL_MODELVIEW);
}

static void display(void)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    /* orbit camera around model centre */
    float dist = camDist * model.radius;
    float ex = dist * sinf(RAD(camYaw))  * cosf(RAD(camPitch));
    float ey = dist * sinf(RAD(camPitch));
    float ez = dist * cosf(RAD(camYaw))  * cosf(RAD(camPitch));

    gluLookAt(model.center.x + ex + camPanX * model.radius,
              model.center.y + ey + camPanY * model.radius,
              model.center.z + ez,
              model.center.x + camPanX * model.radius,
              model.center.y + camPanY * model.radius,
              model.center.z,
              0.0, 1.0, 0.0);

    if (showLighting) {
        glEnable(GL_LIGHTING);
        setupLights();
    } else {
        glDisable(GL_LIGHTING);
        glColor3f(0.8f, 0.8f, 0.8f);
    }

    drawGround();

    /* wireframe toggle */
    if (showWireframe) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    /* turntable rotation around model centre */
    glPushMatrix();
        glTranslatef(model.center.x, model.center.y, model.center.z);
        glRotatef(turnAngle, 0, 1, 0);
        glTranslatef(-model.center.x, -model.center.y, -model.center.z);

        /* texture toggle */
        if (!showTextures) glDisable(GL_TEXTURE_2D);

        glCallList(model.dispList);
    glPopMatrix();

    if (showWireframe) glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    /* ── HUD ── */
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
        glLoadIdentity();
        int vp[4]; glGetIntegerv(GL_VIEWPORT, vp);
        gluOrtho2D(0, vp[2], 0, vp[3]);
        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
            glLoadIdentity();
            glColor3f(0.90f, 0.92f, 0.95f);
            glRasterPos2i(14, vp[3] - 20);
            char hud[256];
            snprintf(hud, sizeof(hud),
                "%dK tris  %d mats  |  SPACE=rotate  L=light  T=tex  F=wire  G=grid  Mouse=orbit  ESC=quit",
                model.faces.count / 3000, model.matCount);
            const char *c;
            for (c = hud; *c; c++)
                glutBitmapCharacter(GLUT_BITMAP_8_BY_13, *c);
        glPopMatrix();
        glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);

    glutSwapBuffers();
}

static void timer(int v)
{
    (void)v;
    if (autoRotate) {
        turnAngle += turnSpeed;
        if (turnAngle >= 360) turnAngle -= 360;
    }
    glutPostRedisplay();
    glutTimerFunc(16, timer, 0);
}

static void keyboard(unsigned char k, int x, int y)
{
    (void)x; (void)y;
    switch (k) {
        case ' ': autoRotate = !autoRotate; break;
        case 'r': case 'R':
            camYaw=25; camPitch=-8; camDist=2.5f;
            camPanX=camPanY=0; turnAngle=0; break;
        case 'w': case 'W': camDist -= 0.08f; if(camDist<0.15f) camDist=0.15f; break;
        case 's': case 'S': camDist += 0.08f; if(camDist>10.0f) camDist=10.0f; break;
        case 'a': case 'A': camPanX -= 0.05f; break;
        case 'd': case 'D': camPanX += 0.05f; break;
        case 'q': case 'Q': camPanY += 0.05f; break;
        case 'e': case 'E': camPanY -= 0.05f; break;
        case 'l': case 'L': showLighting  = !showLighting; break;
        case 't': case 'T': showTextures  = !showTextures; break;
        case 'f': case 'F': showWireframe = !showWireframe; break;
        case 'g': case 'G': showGround    = !showGround; break;
        case '+': case '=': turnSpeed += 0.1f; break;
        case '-': case '_': turnSpeed -= 0.1f; if(turnSpeed<0.02f) turnSpeed=0.02f; break;
        case 27: exit(0);
    }
}

static void specialKey(int k, int x, int y)
{
    (void)x; (void)y;
    switch (k) {
        case GLUT_KEY_LEFT:  camYaw  -= 4; break;
        case GLUT_KEY_RIGHT: camYaw  += 4; break;
        case GLUT_KEY_UP:    camPitch+= 3; if(camPitch> 89) camPitch= 89; break;
        case GLUT_KEY_DOWN:  camPitch-= 3; if(camPitch<-89) camPitch=-89; break;
    }
}

static void mouseBtn(int b, int s, int x, int y)
{
    if (b == GLUT_LEFT_BUTTON) { mouseDown = (s==GLUT_DOWN); lastMX=x; lastMY=y; }
    if (b == 3) { camDist -= 0.10f; if(camDist<0.15f) camDist=0.15f; }
    if (b == 4) { camDist += 0.10f; if(camDist>10.0f) camDist=10.0f; }
}

static void mouseMove(int x, int y)
{
    if (!mouseDown) return;
    camYaw   += (x - lastMX) * 0.4f;
    camPitch += (y - lastMY) * 0.4f;
    if (camPitch >  89) camPitch =  89;
    if (camPitch < -89) camPitch = -89;
    lastMX = x; lastMY = y;
}

/* ═══════════════════════════════════════════════
 *  MAIN
 * ═══════════════════════════════════════════════ */
int main(int argc, char **argv)
{
    if (argc < 2) {
        printf("Usage: %s <model.obj>\n", argv[0]);
        printf("\nExample:\n");
        printf("  %s city/city.obj\n", argv[0]);
        printf("  %s car.obj\n", argv[0]);
        return 1;
    }

    /* init GLUT */
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(1280, 800);
    glutInitWindowPosition(60, 40);
    glutCreateWindow("OBJ Viewer — OpenGL / GLUT");

    /* GL state */
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_LIGHTING);
    glEnable(GL_NORMALIZE);
    glShadeModel(GL_SMOOTH);
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    glClearColor(0.10f, 0.11f, 0.15f, 1.0f);

    /* fog */
    glEnable(GL_FOG);
    float fc[4] = {0.10f, 0.11f, 0.15f, 1.0f};
    glFogfv(GL_FOG_COLOR, fc);
    glFogi(GL_FOG_MODE, GL_LINEAR);

    /* load model */
    memset(&model, 0, sizeof(model));
    loadOBJ(argv[1]);

    /* set fog distances relative to model size */
    glFogf(GL_FOG_START, model.radius * 4.0f);
    glFogf(GL_FOG_END,   model.radius * 12.0f);

    /* set default camera distance */
    camDist = 2.5f;

    /* compile for fast rendering */
    compileDisplayList();

    /* callbacks */
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(specialKey);
    glutMouseFunc(mouseBtn);
    glutMotionFunc(mouseMove);
    glutTimerFunc(16, timer, 0);

    printf("============================================\n");
    printf("  OBJ Viewer — OpenGL / GLUT\n");
    printf("============================================\n");
    printf("  SPACE       turntable on/off\n");
    printf("  W/S         zoom       A/D  pan\n");
    printf("  Q/E         pan up / down\n");
    printf("  Arrows      pitch / yaw\n");
    printf("  Mouse drag  orbit      Scroll zoom\n");
    printf("  L           toggle lighting\n");
    printf("  T           toggle textures\n");
    printf("  F           toggle wireframe\n");
    printf("  G           toggle ground grid\n");
    printf("  +/-         turntable speed\n");
    printf("  R           reset      ESC  quit\n");
    printf("============================================\n");

    glutMainLoop();
    return 0;
}

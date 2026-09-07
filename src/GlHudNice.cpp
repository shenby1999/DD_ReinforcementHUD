#include <windows.h>
#include <stdio.h>
#include <GL/gl.h>
#include <GL/glcorearb.h>

typedef void (WINAPI *Swap_t)(void*);
typedef void (APIENTRY *PFN_glGenVertexArrays)(GLsizei, GLuint*);
typedef void (APIENTRY *PFN_glBindVertexArray)(GLuint);
typedef void (APIENTRY *PFN_glGenBuffers)(GLsizei, GLuint*);
typedef void (APIENTRY *PFN_glBindBuffer)(GLenum, GLuint);
typedef void (APIENTRY *PFN_glBufferData)(GLenum, GLsizeiptr, const void*, GLenum);
typedef void (APIENTRY *PFN_glVertexAttribPointer)(GLuint, GLint, GLenum, GLboolean, GLsizei, const void*);
typedef void (APIENTRY *PFN_glEnableVertexAttribArray)(GLuint);
typedef GLuint (APIENTRY *PFN_glCreateShader)(GLenum);
typedef void (APIENTRY *PFN_glShaderSource)(GLuint, GLsizei, const GLchar* const*, const GLint*);
typedef void (APIENTRY *PFN_glCompileShader)(GLuint);
typedef void (APIENTRY *PFN_glGetShaderiv)(GLuint, GLenum, GLint*);
typedef void (APIENTRY *PFN_glGetShaderInfoLog)(GLuint, GLsizei, GLsizei*, GLchar*);
typedef GLuint (APIENTRY *PFN_glCreateProgram)(void);
typedef void (APIENTRY *PFN_glAttachShader)(GLuint, GLuint);
typedef void (APIENTRY *PFN_glLinkProgram)(GLuint);
typedef void (APIENTRY *PFN_glGetProgramiv)(GLuint, GLenum, GLint*);
typedef void (APIENTRY *PFN_glGetProgramInfoLog)(GLuint, GLsizei, GLsizei*, GLchar*);
typedef void (APIENTRY *PFN_glDeleteShader)(GLuint);
typedef void (APIENTRY *PFN_glUseProgram)(GLuint);
typedef GLint (APIENTRY *PFN_glGetUniformLocation)(GLuint, const GLchar*);
typedef void (APIENTRY *PFN_glUniform4f)(GLint, GLfloat, GLfloat, GLfloat, GLfloat);

static Swap_t g_origSwap = 0;
static bool g_hooked = false;
static GLuint g_program = 0, g_vao = 0, g_vbo = 0;
static GLint g_uColor = -1;

static PFN_glGenVertexArrays glGenVertexArrays;
static PFN_glBindVertexArray glBindVertexArray;
static PFN_glGenBuffers glGenBuffers;
static PFN_glBindBuffer glBindBuffer;
static PFN_glBufferData glBufferData;
static PFN_glVertexAttribPointer glVertexAttribPointer;
static PFN_glEnableVertexAttribArray glEnableVertexAttribArray;
static PFN_glCreateShader glCreateShader;
static PFN_glShaderSource glShaderSource;
static PFN_glCompileShader glCompileShader;
static PFN_glGetShaderiv glGetShaderiv;
static PFN_glGetShaderInfoLog glGetShaderInfoLog;
static PFN_glCreateProgram glCreateProgram;
static PFN_glAttachShader glAttachShader;
static PFN_glLinkProgram glLinkProgram;
static PFN_glGetProgramiv glGetProgramiv;
static PFN_glGetProgramInfoLog glGetProgramInfoLog;
static PFN_glDeleteShader glDeleteShader;
static PFN_glUseProgram glUseProgram;
static PFN_glGetUniformLocation glGetUniformLocation;
static PFN_glUniform4f glUniform4f;

static void LogMsg(const char* msg)
{
    FILE* f = fopen("ReinforcementHudColor.log", "a");
    if (f) { fprintf(f, "%s\n", msg); fclose(f); }
}

static void* GetProc(const char* name)
{
    void* p = (void*)wglGetProcAddress(name);
    if (!p) p = (void*)GetProcAddress(GetModuleHandleA("opengl32.dll"), name);
    return p;
}

static bool LoadGL()
{
    glGenVertexArrays = (PFN_glGenVertexArrays)GetProc("glGenVertexArrays");
    glBindVertexArray = (PFN_glBindVertexArray)GetProc("glBindVertexArray");
    glGenBuffers = (PFN_glGenBuffers)GetProc("glGenBuffers");
    glBindBuffer = (PFN_glBindBuffer)GetProc("glBindBuffer");
    glBufferData = (PFN_glBufferData)GetProc("glBufferData");
    glVertexAttribPointer = (PFN_glVertexAttribPointer)GetProc("glVertexAttribPointer");
    glEnableVertexAttribArray = (PFN_glEnableVertexAttribArray)GetProc("glEnableVertexAttribArray");
    glCreateShader = (PFN_glCreateShader)GetProc("glCreateShader");
    glShaderSource = (PFN_glShaderSource)GetProc("glShaderSource");
    glCompileShader = (PFN_glCompileShader)GetProc("glCompileShader");
    glGetShaderiv = (PFN_glGetShaderiv)GetProc("glGetShaderiv");
    glGetShaderInfoLog = (PFN_glGetShaderInfoLog)GetProc("glGetShaderInfoLog");
    glCreateProgram = (PFN_glCreateProgram)GetProc("glCreateProgram");
    glAttachShader = (PFN_glAttachShader)GetProc("glAttachShader");
    glLinkProgram = (PFN_glLinkProgram)GetProc("glLinkProgram");
    glGetProgramiv = (PFN_glGetProgramiv)GetProc("glGetProgramiv");
    glGetProgramInfoLog = (PFN_glGetProgramInfoLog)GetProc("glGetProgramInfoLog");
    glDeleteShader = (PFN_glDeleteShader)GetProc("glDeleteShader");
    glUseProgram = (PFN_glUseProgram)GetProc("glUseProgram");
    glGetUniformLocation = (PFN_glGetUniformLocation)GetProc("glGetUniformLocation");
    glUniform4f = (PFN_glUniform4f)GetProc("glUniform4f");
    return glGenVertexArrays && glBindVertexArray && glGenBuffers && glBindBuffer && glBufferData &&
        glVertexAttribPointer && glEnableVertexAttribArray && glCreateShader && glShaderSource &&
        glCompileShader && glGetShaderiv && glGetShaderInfoLog && glCreateProgram && glAttachShader &&
        glLinkProgram && glGetProgramiv && glGetProgramInfoLog && glDeleteShader && glUseProgram &&
        glGetUniformLocation && glUniform4f;
}

static GLuint CompileShader(GLenum type, const char* src)
{
    GLuint sh = glCreateShader(type);
    glShaderSource(sh, 1, &src, NULL);
    glCompileShader(sh);
    GLint ok = 0;
    glGetShaderiv(sh, GL_COMPILE_STATUS, &ok);
    if (!ok)
    {
        char log[1024]; GLsizei len=0;
        glGetShaderInfoLog(sh, sizeof(log), &len, log);
        char b[1200]; sprintf(b, "shader compile error: %s", log); LogMsg(b);
        glDeleteShader(sh);
        return 0;
    }
    return sh;
}

static bool InitGL()
{
    if (g_program) return true;
    if (!LoadGL()) { LogMsg("LoadGL failed"); return false; }
    const char* vs = "#version 330 core\nin vec2 pos; void main(){ gl_Position = vec4(pos,0.0,1.0); }";
    const char* fs = "#version 330 core\nuniform vec4 uColor; out vec4 frag; void main(){ frag = uColor; }";
    GLuint v = CompileShader(GL_VERTEX_SHADER, vs);
    GLuint f = CompileShader(GL_FRAGMENT_SHADER, fs);
    if (!v || !f) return false;
    g_program = glCreateProgram();
    glAttachShader(g_program, v);
    glAttachShader(g_program, f);
    glLinkProgram(g_program);
    GLint ok=0; glGetProgramiv(g_program, GL_LINK_STATUS, &ok);
    if (!ok) { char log[1024]; GLsizei len=0; glGetProgramInfoLog(g_program,sizeof(log),&len,log); char b[1200]; sprintf(b,"link error: %s",log); LogMsg(b); return false; }
    glDeleteShader(v); glDeleteShader(f);
    glGenVertexArrays(1, &g_vao);
    glBindVertexArray(g_vao);
    glGenBuffers(1, &g_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, g_vbo);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2*sizeof(float), 0);
    glEnableVertexAttribArray(0);
    g_uColor = glGetUniformLocation(g_program, "uColor");
    LogMsg("InitGL ok");
    return true;
}

static void AddRect(float* &out, float x0, float y0, float x1, float y1, int w, int h)
{
    float X0 = x0/w*2.0f - 1.0f;
    float X1 = x1/w*2.0f - 1.0f;
    float Y0 = 1.0f - y0/h*2.0f;
    float Y1 = 1.0f - y1/h*2.0f;
    out[0]=X0; out[1]=Y0; out[2]=X1; out[3]=Y0; out[4]=X0; out[5]=Y1;
    out[6]=X0; out[7]=Y1; out[8]=X1; out[9]=Y0; out[10]=X1; out[11]=Y1;
    out += 12;
}

static void AddSegments(float* &out, int digit, float ox, float oy, float w, float h, int viewW, int viewH)
{
    float thick = h*0.08f;
    float hx0=ox, hx1=ox+w;
    float vx0=ox, vx1=ox+thick;
    float vx2=ox+w-thick, vx3=ox+w;
    float yTop=oy, yBot=oy+h-thick;
    float yMid=oy+h/2-thick/2;
    float yTopEnd=oy+thick, yMidTop=oy+h/2-thick/2, yMidBot=oy+h/2+thick/2, yBotStart=oy+h-thick;
    bool seg[7]={false};
    switch(digit){
        case 0: seg[0]=seg[1]=seg[2]=seg[3]=seg[4]=seg[5]=true; break;
        case 1: seg[1]=seg[2]=true; break;
        case 2: seg[0]=seg[1]=seg[6]=seg[4]=seg[3]=true; break;
        case 3: seg[0]=seg[1]=seg[6]=seg[2]=seg[3]=true; break;
        case 4: seg[5]=seg[6]=seg[1]=seg[2]=true; break;
        case 5: seg[0]=seg[5]=seg[6]=seg[2]=seg[3]=true; break;
        case 6: seg[0]=seg[5]=seg[6]=seg[2]=seg[3]=seg[4]=true; break;
        case 7: seg[0]=seg[1]=seg[2]=true; break;
        case 8: for(int i=0;i<7;i++)seg[i]=true; break;
        case 9: seg[0]=seg[1]=seg[2]=seg[3]=seg[5]=seg[6]=true; break;
    }
    if(seg[0]) AddRect(out,hx0,yTop,hx1,yTopEnd,viewW,viewH);
    if(seg[3]) AddRect(out,hx0,yBot,hx1,yBot+thick,viewW,viewH);
    if(seg[6]) AddRect(out,hx0,yMid,hx1,yMid+thick,viewW,viewH);
    if(seg[5]) AddRect(out,vx0,yTopEnd,vx1,yMidTop,viewW,viewH);
    if(seg[1]) AddRect(out,vx2,yTopEnd,vx3,yMidTop,viewW,viewH);
    if(seg[4]) AddRect(out,vx0,yMidBot,vx1,yBotStart,viewW,viewH);
    if(seg[2]) AddRect(out,vx2,yMidBot,vx3,yBotStart,viewW,viewH);
}

static void DrawVerts(const float* verts, size_t vertCount, float r, float g, float b, float a)
{
    glUseProgram(g_program);
    glBindVertexArray(g_vao);
    glBindBuffer(GL_ARRAY_BUFFER, g_vbo);
    glBufferData(GL_ARRAY_BUFFER, vertCount * sizeof(float), verts, GL_STREAM_DRAW);
    glUniform4f(g_uColor, r, g, b, a);
    glDrawArrays(GL_TRIANGLES, 0, (GLsizei)(vertCount / 2));
}

static void DrawOverlay()
{
    BYTE* base = (BYTE*)GetModuleHandleA(NULL);
    if (!base) return;
    uintptr_t app = *(uintptr_t*)(base + 0x1179A08);
    int count=0, summon=4, flag=0;
    if (app) { count = *(int*)(app + 0x4B7C); flag = *(int*)(app + 0x4B80); summon = *(int*)(base + 0x2AC78B4); }
    // If the stall flag is active, the game may accelerate the count by a large
    // step on the next stall round. In that state a count of 1 can already mean
    // "reinforcements will trigger next round", so show 1 instead of 3.
    int rem;
    if (flag != 0 && count > 0) {
        rem = 1;
    } else {
        rem = summon - count;
    }
    if (rem < 0) rem = 0;

    float nr=1.0f, ng=0.75f, nb=0.1f; // amber/gold default
    if (rem == 2) { nr=1.0f; ng=0.55f; nb=0.0f; }      // orange
    else if (rem <= 1) { nr=1.0f; ng=0.1f; nb=0.05f; }  // bright red

    GLint vp[4]={0};
    glGetIntegerv(GL_VIEWPORT, vp);
    int w=vp[2]?vp[2]:853, h=vp[3]?vp[3]:480;
    if (!InitGL()) return;

    // active == true means the game is actually accumulating a stall count.
    // active == false means no stall count yet / no reinforcements detected.
    bool active = count > 0;

    float cx = w/2.0f;
    // Scale all fixed HUD sizes relative to 720p so it looks consistent at other resolutions.
    float s = (h / 720.0f) * 0.20f;
    float digitW = 110.0f * s, digitH = 200.0f * s;
    float ox = cx - digitW/2.0f;
    float oy = 20.0f * s;
    float padX = 130.0f * s;
    float border = 8.0f * s;
    float shadowOff = 4.0f * s;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    float verts[4096];
    float* p = verts;

    // Background panel
    AddRect(p, cx-padX, oy-border, cx+padX, oy+digitH+border, w, h);
    size_t bgCount = p - verts;
    DrawVerts(verts, bgCount, 0.03f, 0.02f, 0.02f, 0.55f);

    if (active) {
        // Black shadow digit
        p = verts;
        AddSegments(p, rem, ox+shadowOff, oy+shadowOff, digitW, digitH, w, h);
        size_t shadowCount = p - verts;
        DrawVerts(verts, shadowCount, 0.0f, 0.0f, 0.0f, 0.9f);

        // Amber digit
        p = verts;
        AddSegments(p, rem, ox, oy, digitW, digitH, w, h);
        size_t digitCount = p - verts;
        DrawVerts(verts, digitCount, nr, ng, nb, 1.0f);

        // Remaining pips below the digit
        float pipY = oy + digitH + 16.0f * s;
        float pipW = 22.0f * s, pipH = 8.0f * s, gap = 8.0f * s;
        float total = 4*pipW + 3*gap;
        float startX = cx - total/2.0f;
        p = verts;
        for (int i=0; i<4; i++) {
            float x0 = startX + i*(pipW+gap);
            AddRect(p, x0, pipY, x0+pipW, pipY+pipH, w, h);
        }
        size_t pipBgCount = p - verts;
        DrawVerts(verts, pipBgCount, 0.0f, 0.0f, 0.0f, 0.6f);

        p = verts;
        for (int i=0; i<rem; i++) {
            float x0 = startX + i*(pipW+gap);
            AddRect(p, x0, pipY, x0+pipW, pipY+pipH, w, h);
        }
        size_t pipCount = p - verts;
        if (pipCount) DrawVerts(verts, pipCount, nr, ng, nb, 1.0f);
    } else {
        // Draw two amber dashes to represent "--" (no active reinforcement count yet).
        float dashW = 34.0f * s;
        float dashH = 16.0f * s;
        float dashGap = 22.0f * s;
        float dashY = oy + digitH/2.0f - dashH/2.0f;
        float dashTotal = 2*dashW + dashGap;
        float x0 = cx - dashTotal/2.0f;
        p = verts;
        AddRect(p, x0, dashY, x0+dashW, dashY+dashH, w, h);
        AddRect(p, x0+dashW+dashGap, dashY, x0+2*dashW+dashGap, dashY+dashH, w, h);
        size_t dashCount = p - verts;
        DrawVerts(verts, dashCount, nr, ng, nb, 1.0f);
    }

    glDisable(GL_BLEND);
    glBindVertexArray(0);
}

static void WINAPI Hook(void* window)
{
    DrawOverlay();
    if (g_origSwap) g_origSwap(window);
}

static void* FindIATEntry()
{
    BYTE* base=(BYTE*)GetModuleHandleA(NULL);
    IMAGE_DOS_HEADER* dos=(IMAGE_DOS_HEADER*)base;
    IMAGE_NT_HEADERS* nt=(IMAGE_NT_HEADERS*)(base+dos->e_lfanew);
    IMAGE_DATA_DIRECTORY d=nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    IMAGE_IMPORT_DESCRIPTOR* desc=(IMAGE_IMPORT_DESCRIPTOR*)(base+d.VirtualAddress);
    for(;desc->Name;desc++){
        const char* nm=(const char*)(base+desc->Name);
        if(_stricmp(nm,"SDL2.dll"))continue;
        IMAGE_THUNK_DATA64* oft=(IMAGE_THUNK_DATA64*)(base+desc->OriginalFirstThunk);
        IMAGE_THUNK_DATA64* ft=(IMAGE_THUNK_DATA64*)(base+desc->FirstThunk);
        for(int i=0;;i++){
            if(!oft[i].u1.AddressOfData && !ft[i].u1.Function) break;
            if(!(oft[i].u1.Ordinal&IMAGE_ORDINAL_FLAG64)){
                IMAGE_IMPORT_BY_NAME* n=(IMAGE_IMPORT_BY_NAME*)(base+oft[i].u1.AddressOfData);
                if(!strcmp((char*)n->Name,"SDL_GL_SwapWindow")) return &ft[i].u1.Function;
            }
        }
        break;
    }
    return 0;
}

BOOL APIENTRY DllMain(HMODULE h, DWORD r, LPVOID)
{
    if(r==DLL_PROCESS_ATTACH){
        DisableThreadLibraryCalls(h);
        void* slot=FindIATEntry();
        if(slot){
            g_origSwap=*(Swap_t*)slot;
            DWORD op; VirtualProtect(slot,8,PAGE_READWRITE,&op);
            *(Swap_t*)slot=Hook;
            VirtualProtect(slot,8,op,&op);
            g_hooked=true;
            LogMsg("GlHudNice hook installed");
        }
    }
    return TRUE;
}

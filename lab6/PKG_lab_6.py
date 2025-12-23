import pygame
from pygame.locals import *
from OpenGL.GL import *
from OpenGL.GLU import *
import numpy as np
import tkinter as tk
from tkinter import messagebox

# --- GUI FUNCTIONS ---
def show_startup_instructions():
    root = tk.Tk()
    root.withdraw()
    messagebox.showinfo("Controls Guide", 
        "MOVEMENT:\n"
        "W / S : Move along X-axis\n"
        "A / D : Move along Y-axis\n"
        "UP / DOWN : Move along Z-axis\n\n"
        "ROTATION:\n"
        "1, 2, 3 : Select Axis (X, Y, Z)\n"
        "LEFT / RIGHT : Rotate active axis\n\n"
        "SCALE:\n"
        "'+' / '-' : Zoom In / Out")
    root.destroy()

class MatrixWindow:
    def __init__(self):
        self.root = tk.Tk()
        self.root.title("Transformation Matrix")
        self.root.attributes("-topmost", True)
        self.label = tk.Label(self.root, font=('Courier', 12), justify=tk.LEFT, padx=10, pady=10)
        self.label.pack()
        self.update_matrix(np.identity(4))

    def update_matrix(self, matrix):
        formatted_map = np.around(matrix, 3)
        self.label.config(text=str(formatted_map))
        self.root.update()

# --- GEOMETRY: LETTER "M" ---
def get_letter_m_data():
    v = [
        [0, 0, 0], [0, 0, 4], [1.2, 0, 4], [2, 0, 2], [2.8, 0, 4], [4, 0, 4],
        [4, 0, 0], [3, 0, 0], [3, 0, 2.5], [2, 0, 1], [1, 0, 2.5], [1, 0, 0]
    ]
    t = 0.6
    verts = []
    for p in v: verts.append([p[0], -t / 2, p[2]]) 
    for p in v: verts.append([p[0], t / 2, p[2]])
    n = len(v)
    edges = []
    loop = [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 0]
    for i in range(len(loop) - 1):
        idx1, idx2 = loop[i], loop[i+1]
        edges.append((idx1, idx2))
        edges.append((idx1 + n, idx2 + n))
        edges.append((idx1, idx1 + n))
    return np.array(verts, dtype='float32'), edges

VERTS, EDGES = get_letter_m_data()

# --- OPENGL HELPERS ---
def draw_stroke_char(char):
    glBegin(GL_LINES)
    if char == 'X':
        glVertex2f(-0.2, -0.2); glVertex2f(0.2, 0.2)
        glVertex2f(0.2, -0.2); glVertex2f(-0.2, 0.2)
    elif char == 'Y':
        glVertex2f(-0.2, 0.2); glVertex2f(0, 0)
        glVertex2f(0.2, 0.2); glVertex2f(0, 0)
        glVertex2f(0, 0); glVertex2f(0, -0.2)
    elif char == 'Z':
        glVertex2f(-0.2, 0.2); glVertex2f(0.2, 0.2)
        glVertex2f(0.2, 0.2); glVertex2f(-0.2, -0.2)
        glVertex2f(-0.2, -0.2); glVertex2f(0.2, -0.2)
    glEnd()

def draw_labels(view_name):
    glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity(); glOrtho(0, 10, 0, 10, -1, 1)
    glMatrixMode(GL_MODELVIEW); glPushMatrix(); glLoadIdentity()
    glColor3f(0, 0, 0); glLineWidth(2); glTranslatef(0.5, 9.2, 0)
    for c in view_name:
        draw_stroke_char(c); glTranslatef(0.5, 0, 0)
    glPopMatrix(); glMatrixMode(GL_PROJECTION); glPopMatrix(); glMatrixMode(GL_MODELVIEW)

def draw_grid(plane):
    glLineWidth(1); glColor3f(0.85, 0.85, 0.85)
    glBegin(GL_LINES)
    for i in range(-10, 11):
        if plane == 'XY':
            glVertex3f(i, -10, 0); glVertex3f(i, 10, 0)
            glVertex3f(-10, i, 0); glVertex3f(10, i, 0) # Fixed grid lines
        elif plane == 'XZ':
            glVertex3f(i, 0, -10); glVertex3f(i, 0, 10)
            glVertex3f(-10, 0, i); glVertex3f(10, 0, i) # Fixed grid lines
        elif plane == 'YZ':
            glVertex3f(0, i, -10); glVertex3f(0, i, 10)
            glVertex3f(0, -10, i); glVertex3f(0, 10, i) # Fixed grid lines
    glEnd()

def draw_axes_with_labels():
    glLineWidth(2)
    glBegin(GL_LINES)
    glColor3f(1, 0, 0); glVertex3f(0, 0, 0); glVertex3f(5, 0, 0)
    glColor3f(0, 0.8, 0); glVertex3f(0, 0, 0); glVertex3f(0, 5, 0)
    glColor3f(0, 0, 1); glVertex3f(0, 0, 0); glVertex3f(0, 0, 5)
    glEnd()

def get_full_matrix(pos, angles, scale):
    T = np.identity(4); T[:3, 3] = pos
    def rot_m(a, ax):
        r = np.radians(a); c, s = np.cos(r), np.sin(r)
        m = np.identity(4)
        if ax == 0: m[1, 1], m[1, 2], m[2, 1], m[2, 2] = c, -s, s, c
        elif ax == 1: m[0, 0], m[0, 2], m[2, 0], m[2, 2] = c, s, -s, c
        elif ax == 2: m[0, 0], m[0, 1], m[1, 0], m[1, 1] = c, -s, s, c
        return m
    R = rot_m(angles[0], 0) @ rot_m(angles[1], 1) @ rot_m(angles[2], 2)
    S = np.identity(4); np.fill_diagonal(S, [scale, scale, scale, 1])
    return T @ R @ S

# --- MAIN ---
def main():
    show_startup_instructions()
    matrix_gui = MatrixWindow()

    pygame.init()
    screen_size = (1000, 800)
    pygame.display.set_mode(screen_size, DOUBLEBUF | OPENGL)
    pygame.display.set_caption("3D Letter M Transformation")
    
    pos, angles, scale, active_axis = np.array([0.0, 0.0, 0.0]), [0.0, 0.0, 0.0], 1.0, 2
    clock = pygame.time.Clock()

    while True:
        for event in pygame.event.get():
            if event.type == QUIT: 
                matrix_gui.root.destroy()
                pygame.quit()
                return
            if event.type == KEYDOWN:
                if event.key == K_1: active_axis = 0
                if event.key == K_2: active_axis = 1
                if event.key == K_3: active_axis = 2

        keys = pygame.key.get_pressed()
        if keys[K_s]: pos[0] += 0.1
        if keys[K_w]: pos[0] -= 0.1
        if keys[K_a]: pos[1] -= 0.1
        if keys[K_d]: pos[1] += 0.1
        if keys[K_UP]: pos[2] += 0.1
        if keys[K_DOWN]: pos[2] -= 0.1
        if keys[K_LEFT]: angles[active_axis] += 3
        if keys[K_RIGHT]: angles[active_axis] -= 3
        if keys[K_EQUALS]: scale += 0.02
        if keys[K_MINUS]: scale = max(0.1, scale - 0.02)

        mat = get_full_matrix(pos, angles, scale)
        matrix_gui.update_matrix(mat)

        w_v, h_v = screen_size[0] // 2, screen_size[1] // 2
        views = [
            (0, h_v, 'OXY', (0, 0, 12, 0, 0, 0, 0, 1, 0), 'XY'),
            (w_v, h_v, 'OYZ', (12, 0, 0, 0, 0, 0, 0, 0, 1), 'YZ'),
            (0, 0, 'OXZ', (0, 12, 0, 0, 0, 0, 1, 0, 0), 'XZ'),
            (w_v, 0, '3D', (10, 8, 10, 0, 0, 0, 0, 0, 1), 'XY')
        ]

        for vx, vy, name, cam, grid_p in views:
            glViewport(vx, vy, w_v, h_v); glEnable(GL_SCISSOR_TEST); glScissor(vx, vy, w_v, h_v)
            glClearColor(1, 1, 1, 1); glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)
            glMatrixMode(GL_PROJECTION); glLoadIdentity()
            if name == '3D': gluPerspective(45, w_v / h_v, 0.1, 100)
            else: glOrtho(-8, 8, -8, 8, -50, 50)
            glMatrixMode(GL_MODELVIEW); glLoadIdentity(); gluLookAt(*cam)

            draw_grid(grid_p)
            draw_axes_with_labels()
            draw_labels(name)

            glPushMatrix()
            glMultMatrixf(mat.T)
            glTranslatef(-2.0, 0, -2.0)
            glLineWidth(3); glColor3f(0.2, 0.4, 0.9)
            glBegin(GL_LINES)
            for e in EDGES:
                for v_idx in e: glVertex3fv(VERTS[v_idx])
            glEnd()
            glPopMatrix(); glDisable(GL_SCISSOR_TEST)

        pygame.display.flip()
        clock.tick(60)

if __name__ == "__main__":
    main()
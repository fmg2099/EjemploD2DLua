# EjemploD2DLua

Microengine gráfico para dibujar figuras geométricas simples con el lenguaje lua.
El proyecto integra WinAPI para crear la ventana, Direct2D para los gráficos y lua para exponer las funciones de dibujado.

Hasta el momento se exponen las siguientes funciones:
	SimpleDraw.Clear(int R, int G, int B)
	SimpleDraw.SetBrushColor(int R, int G, int B)
    SimpleDraw.DrawLine(float x, float y, float x2, float y2)
	simpledraw.DrawCircle(float x, float y, float radius)
    SimpleDraw.DrawRect(float x, float y, float width, float height)
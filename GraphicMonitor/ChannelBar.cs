using System;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.Windows.Forms;

public class OrionChannelBar : Control
{
    private int percent;
    private double displayedPercent;
    private bool hasValue;
    private bool live;
    public int Percent {
        get { return percent; }
        set {
            percent = Math.Max(-100, Math.Min(100, value));
            if (!hasValue || !live) { displayedPercent = percent; hasValue = true; Invalidate(); }
        }
    }
    public double DisplayedPercent { get { return displayedPercent; } }
    public bool Live {
        get { return live; }
        set {
            if (live != value) {
                live = value;
                // A stale display freezes on the last actual reading.
                if (!live) SnapToTarget();
                Invalidate();
            }
        }
    }
    public void SnapToTarget() { displayedPercent = percent; Invalidate(); }
    public void AdvanceAnimation(double elapsedSeconds) {
        if (!live || !hasValue || elapsedSeconds <= 0) return;
        double before = displayedPercent;
        // Time-based easing: 45 ms time constant, independent of timer jitter.
        double alpha = 1 - Math.Exp(-elapsedSeconds / 0.045);
        displayedPercent += (percent - displayedPercent) * alpha;
        if (Math.Abs(percent - displayedPercent) < 0.05) displayedPercent = percent;
        if (Math.Abs(before - displayedPercent) > 0.001) Invalidate();
    }
    public Color Accent { get; set; }
    public OrionChannelBar()
    {
        SetStyle(ControlStyles.UserPaint | ControlStyles.AllPaintingInWmPaint |
                 ControlStyles.OptimizedDoubleBuffer | ControlStyles.ResizeRedraw, true);
        Accent = Color.DeepSkyBlue;
        BackColor = Color.FromArgb(32,42,58);
        Size = new Size(100,180);
    }
    protected override void OnPaint(PaintEventArgs e)
    {
        base.OnPaint(e);
        Graphics g = e.Graphics;
        g.SmoothingMode = SmoothingMode.AntiAlias;
        int top = 12, bottom = Math.Max(top+2, Height-12);
        int w = Math.Max(12, Math.Min(42, Width-48));
        Rectangle track = new Rectangle((Width-w)/2, top, w, bottom-top);
        using (Brush b = new SolidBrush(Color.FromArgb(15,22,34))) g.FillRectangle(b,track);
        int y = bottom - (int)Math.Round((displayedPercent+100)/200.0 * track.Height);
        Color c = live ? Accent : Color.FromArgb(99,112,132);
        if (y < bottom)
        {
            Rectangle fill = new Rectangle(track.X,y,track.Width,bottom-y);
            Color dark = Color.FromArgb(c.R/2,c.G/2,c.B/2);
            using (Brush b = new LinearGradientBrush(track,c,dark,90f)) g.FillRectangle(b,fill);
        }
        using (Pen p = new Pen(Color.FromArgb(75,92,115)))
        {
            for (int i=0; i<=4; i++)
            {
                int tick = top + track.Height*i/4;
                g.DrawLine(p,track.Left-5,tick,track.Right+5,tick);
            }
        }
        int center = top+track.Height/2;
        using (Pen p = new Pen(Color.FromArgb(210,225,240),1))
        { p.DashStyle=DashStyle.Dash; g.DrawLine(p,track.Left-7,center,track.Right+7,center); }
        using (Pen p = new Pen(live ? Color.White : Color.Gray,3))
            g.DrawLine(p,track.Left-3,y,track.Right+3,y);
        using (Font f = new Font("Segoe UI",7))
        using (Brush b = new SolidBrush(Color.FromArgb(155,172,193)))
        {
            g.DrawString("+100",f,b,0,top-5);
            g.DrawString("0",f,b,4,center-6);
            g.DrawString("-100",f,b,0,bottom-10);
        }
    }
}

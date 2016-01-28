class CSaveDC {
    public:
       CSaveDC(CDC & dc) { sdc = &dc; saved = dc.SaveDC(); }
       CSaveDC(CDC * dc) { sdc = dc; saved = dc->SaveDC(); }
       virtual ~CSaveDC() { sdc->RestoreDC(saved); }
    protected:
       CDC * sdc;
       int saved;
};

#ifndef APPS_ZOOM_PARAMETER_CONTROLLER_H
#define APPS_ZOOM_PARAMETER_CONTROLLER_H

#include <escher.h>
#include "interactive_curve_view_range.h"
#include "curve_view.h"

class ZoomParameterController : public ViewController {
public:
  ZoomParameterController(Responder * parentResponder, InteractiveCurveViewRange * interactiveCurveViewRange, CurveView * curveView);
  const char * title() const override;
  View * view() override;
  bool handleEvent(Ion::Events::Event event) override;
  void didBecomeFirstResponder() override;
private:
  class ContentView : public View {
  public:
    ContentView(CurveView * curveView);
    void layoutSubviews() override;
    CurveView * curveView();
  private:
    class LegendView : public View {
    public:
      LegendView();
      void drawRect(KDContext * ctx, KDRect rect) const override;
    private:
      constexpr static int k_numberOfLegends = 6;
      constexpr static KDCoordinate k_tokenWidth = 30;
      void layoutSubviews() override;
      int numberOfSubviews() const override;
      View * subviewAtIndex(int index) override;
      PointerTextView m_legends[k_numberOfLegends];
    };
    int numberOfSubviews() const override;
    View * subviewAtIndex(int index) override;
    CurveView * m_curveView;
    LegendView m_legendView;
    constexpr static KDCoordinate k_legendHeight = 50;
  };
  ContentView m_contentView;
  InteractiveCurveViewRange * m_interactiveRange;
};

#endif

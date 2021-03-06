#ifndef URCHINENGINE_TEXT_H
#define URCHINENGINE_TEXT_H

#include <string>
#include <memory>

#include "scene/GUI/widget/Widget.h"
#include "scene/GUI/widget/Position.h"
#include "resources/font/Font.h"
#include "utils/display/quad/QuadDisplayer.h"

namespace urchin
{
	
	class Text : public Widget
	{
		public:
			Text(Position, const std::string &);
			~Text() override;
		
			void createOrUpdateWidget() override;

			void setText(const std::string &, int maxLength=-1);
			const std::string &getText() const;
			const Font *getFont();
			
			void display(int, float) override;

		private:
			std::string cutText(const std::string &, unsigned int);

			//properties
			std::string text;
			int maxLength;

			//data
			std::vector<std::string> cutTextLines;
			std::vector<int> vertexData;
			std::vector<float> textureData;

			//visual
			Font *font;
			std::unique_ptr<QuadDisplayerBuilder> quadDisplayerBuilder;
			std::shared_ptr<QuadDisplayer> quadDisplayer;
	};
	
}

#endif

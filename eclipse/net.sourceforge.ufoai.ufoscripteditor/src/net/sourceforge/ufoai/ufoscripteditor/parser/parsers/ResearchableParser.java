package net.sourceforge.ufoai.ufoscripteditor.parser.parsers;

import net.sourceforge.ufoai.ufoscripteditor.parser.IParserContext;
import net.sourceforge.ufoai.ufoscripteditor.parser.IUFOParser;
import net.sourceforge.ufoai.ufoscripteditor.parser.IUFOParserFactory;

public class ResearchableParser extends AbstractUFOParser {
	public static final IUFOParserFactory FACTORY = new UFOParserFactoryAdapter() {
		@Override
		public IUFOParser create(IParserContext ctx) {
			return new ResearchableParser(ctx);
		}

		@Override
		public String getID() {
			return "researchable";
		}
	};

	public ResearchableParser(IParserContext ctx) {
		super(ctx);
	}

	@Override
	public boolean hasOnlyValue() {
		return true;
	}
}
